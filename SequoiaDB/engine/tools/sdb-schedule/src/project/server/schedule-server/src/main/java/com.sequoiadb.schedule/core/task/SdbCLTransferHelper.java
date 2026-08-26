/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = SdbCLTransferHelper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.task;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleCommonTools;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SdbCLTransferHelper {
    private static Logger logger = LoggerFactory.getLogger(SdbCLTransferHelper.class);
    private final RunningCallback runningCallback;
    private static final int BATCH_SIZE = 1000;
    private static final int BUFFER_SIZE = 8192;

    public enum CompareLobResult {
        SAME,
        LOB_MODIFY,
        SOURCE_NEW,
        NOT_USER_DATA,
        TARGET_RESIDUE;
    }

    private DBCollection sourceCL;
    private DBCollection targetCL;
    private String sourceSite;
    private String targetSite;

    private boolean deleteMoreLobInTarget;

    private long successRecordNum = 0;
    private long failedRecordNum = 0;
    private long successLobNum = 0;
    private long failedLobNum = 0;
    private static final int DATE_STEP = 20; // seconds

    private volatile Date preDate = new Date();
    private String taskId;
    private long maxExecTime;
    private long taskStartTime;
    private long transferRecordSize = 0;
    private long transferLobSize = 0;

    public SdbCLTransferHelper(String taskId, long maxExecTime, long taskStartTime,
            String sourceSite, String targetSite, DBCollection sourceCL, DBCollection targetCL,
            boolean deleteMoreLobInTarget, RunningCallback runningCallback) {
        this.taskId = taskId;
        this.maxExecTime = maxExecTime;
        this.taskStartTime = taskStartTime;
        this.sourceCL = sourceCL;
        this.targetCL = targetCL;
        this.sourceSite = sourceSite;
        this.targetSite = targetSite;
        this.deleteMoreLobInTarget = deleteMoreLobInTarget;
        this.runningCallback = runningCallback;
    }

    /**
     *
     * @return true: all record already transfer, false: transfer timeout
     */
    public boolean transferRecord() throws ScheduleServerException {
        logger.info("start transferring records.sourceSite={}, targetSite={}, cl={}, taskId={}",
                sourceSite, targetSite, sourceCL.getFullName(), taskId);
        try {
            if (sourceCL.queryOne() == null) {
                return true;
            }
            return doTransferRecord();
        }
        catch (Exception e) {
            throw new ScheduleSystemException("transfer record failed, sourceSite=" + sourceSite
                    + ", targetSite=" + targetSite + ", cl=" + sourceCL.getFullName(), e);
        }
        finally {
            updateProgressNow();
        }
    }

    private boolean doTransferRecord() throws ScheduleServerException {
        BSONObject matcher = null;
        BSONObject lastStatus = getTransferRecordStatus(sourceCL.getFullName());
        if (lastStatus != null) {
            String startId = BsonUtils.getStringChecked(lastStatus,
                    FieldName.CollectionTransferRecordStatus.FIELD_RECORD_START_ID);
            String endId = BsonUtils.getStringChecked(lastStatus,
                    FieldName.CollectionTransferRecordStatus.FIELD_RECORD_END_ID);
            // 检查目标端是否已经存在lastInsertEndId记录, 并且补充插入缺失的记录
            checkAndInsertMissingRecords(startId, endId);
            matcher = new BasicBSONObject("_id", new BasicBSONObject("$gt", new ObjectId(endId)));
        }

        try (DBCursor cursor = sourceCL.query(matcher, null, new BasicBSONObject("_id", 1), null)) {
            List<BSONObject> batch = new ArrayList<>();
            while (cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                batch.add(record);
                if (batch.size() >= BATCH_SIZE) {
                    persistBatch(batch);
                    batch.clear();
                    if (isTimeout() || !isRunning()) {
                        return false;
                    }
                }
            }

            if (!batch.isEmpty()) {
                persistBatch(batch);
            }
        }
        return true;
    }

    private boolean isRunning() {
        if (runningCallback != null) {
            return runningCallback.isRunning();
        }
        return true;
    }

    private void persistBatch(List<BSONObject> batch) throws ScheduleServerException {
        if (batch == null || batch.isEmpty()) {
            return;
        }
        String startId = batch.get(0).get("_id").toString();
        String endId = batch.get(batch.size() - 1).get("_id").toString();

        upsertRecordStatus(sourceCL.getFullName(), startId, endId);
        bulkInsertInTarget(batch);
        updateProgress();
    }

    private void updateProgressNow() {
        try {
            ScheduleServer.getInstance().updateTaskTransferProgress(taskId, sourceCL.getFullName(),
                    successRecordNum, failedRecordNum, transferRecordSize, successLobNum,
                    failedLobNum, transferLobSize);
        }
        catch (Exception e) {
            logger.warn("updateProgress failed", e);
        }
    }

    private void updateProgress() {
        try {
            Date date = new Date();
            int seconds = ScheduleCommonTools.getDuration(preDate, date);
            if (seconds > DATE_STEP) {
                ScheduleServer.getInstance().updateTaskTransferProgress(taskId,
                        sourceCL.getFullName(), successRecordNum, failedRecordNum,
                        transferRecordSize, successLobNum, failedLobNum, transferLobSize);
                preDate = date;
            }
        }
        catch (Exception e) {
            logger.warn("updateProgress failed", e);
        }
    }

    private void bulkInsertInTarget(List<BSONObject> records) {
        try {
            targetCL.bulkInsert(records);
            successRecordNum += records.size();
            records.forEach(record -> transferRecordSize += record.toString().getBytes().length);
        }
        catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                // 插入失败，出现唯一键冲突，就改为逐条插入
                logger.warn(
                        "duplicate key detected in bulk insert, try to insert one by one, sourceSite={}, targetSite={}, cl={}",
                        sourceSite, targetSite, targetCL.getFullName());
                insertOneByOne(records);
            }
            else {
                throw e;
            }
        }
    }

    private void insertOneByOne(List<BSONObject> records) {
        for (BSONObject record : records) {
            try {
                targetCL.insertRecord(record);
                successRecordNum++;
                transferRecordSize += record.toString().getBytes().length;
            }
            catch (BaseException ex) {
                if (ex.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                    logger.debug(
                            "Record duplicate, skip insert, sourceSite={}, targetSite={}, cl={}, record={}",
                            sourceSite, targetSite, targetCL.getFullName(), record);
                }
                else {
                    throw ex;
                }
            }
        }
    }

    private void checkAndInsertMissingRecords(String startId, String endId) {
        BasicBSONList andList = new BasicBSONList();
        BSONObject startMatcher = new BasicBSONObject();
        startMatcher.put("_id", new BasicBSONObject("$gte", new ObjectId(startId)));
        BSONObject endMatcher = new BasicBSONObject();
        endMatcher.put("_id", new BasicBSONObject("$lte", new ObjectId(endId)));
        andList.add(startMatcher);
        andList.add(endMatcher);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", andList);

        Map<ObjectId, BSONObject> idRecordMap = new HashMap<>();
        List<ObjectId> idList = new ArrayList<>();
        try (DBCursor cursor = sourceCL.query(matcher, null, new BasicBSONObject("_id", 1), null)) {
            while (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                ObjectId id = (ObjectId) obj.get("_id");
                idList.add(id);
                idRecordMap.put(id, obj);
            }
        }
        if (idList.isEmpty()) {
            return;
        }
        try (DBCursor targetCursor = targetCL.query(
                new BasicBSONObject("_id", new BasicBSONObject("$in", idList)), null,
                new BasicBSONObject("_id", 1), null)) {
            while (targetCursor.hasNext()) {
                BSONObject obj = targetCursor.getNext();
                ObjectId id = (ObjectId) obj.get("_id");
                idList.remove(id);
            }
        }

        if (idList.isEmpty()) {
            return;
        }

        List<BSONObject> missing = new ArrayList<>();
        for (ObjectId objectId : idList) {
            if (objectId != null) {
                BSONObject record = idRecordMap.get(objectId);
                missing.add(record);
            }
        }

        bulkInsertInTarget(missing);
    }

    private BSONObject getTransferRecordStatus(String clName) throws ScheduleServerException {
        try {
            return ScheduleServer.getInstance().getClTransferRecordStatus(sourceSite, targetSite,
                    clName);
        }
        catch (Exception e) {
            throw new ScheduleSystemException(
                    "getTransferRecordStatus failed, failed to query cl transfer record status, sourceSite="
                            + sourceSite + ", targetSite=" + targetSite + ", clName=" + sourceCL,
                    e);
        }
    }

    private void upsertRecordStatus(String clName, String startId, String endId)
            throws ScheduleServerException {
        try {
            ScheduleServer.getInstance().upsertClTransferRecordStatus(sourceSite, targetSite,
                    clName, startId, endId);
        }
        catch (Exception e) {
            throw new ScheduleSystemException(
                    "updateRecordStatus failed, failed to insert transfer status, transfer record failed, sourceSite="
                            + sourceSite + ", targetSite=" + targetSite + ", clName=" + sourceCL,
                    e);
        }
    }

    /**
     *
     * @return true: all lob already transfer, false: transfer timeout or have lob transfer failed
     */
    public boolean transferLob() throws ScheduleServerException {
        logger.info("start transferring lobs, sourceSite={}, targetSite={}, cl={}, taskId={}", sourceSite,
                targetSite, sourceCL.getFullName(), taskId);
        try {
            return doTransferLob();
        }
        catch (Exception e) {
            throw new ScheduleSystemException("transfer lob failed, sourceSite=" + sourceSite
                    + ", targetSite=" + targetSite + ", cl=" + sourceCL.getFullName(), e);
        }
        finally {
            updateProgressNow();
        }

    }

    private boolean doTransferLob() {
        BSONObject sSort = new BasicBSONObject("CreateTime", 1);
        sSort.put("Oid", 1);

        BSONObject tSort = new BasicBSONObject("UserData.CreateTime", 1);
        tSort.put("Oid", 1);
        try (DBCursor sourceCursor = sourceCL.listLobs(null, null, sSort, null, 0, -1);
                DBCursor targetCursor = targetCL.listLobs(null, null, tSort, null, 0, -1);) {
            BSONObject sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
            BSONObject targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;

            while (sourceLob != null && targetLob != null) {
                CompareLobResult compareLobResult = compareLobInfo(sourceLob, targetLob);
                if (compareLobResult == CompareLobResult.SAME) {
                    sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                }
                else if (compareLobResult == CompareLobResult.LOB_MODIFY) {
                    // 删除目标LOB，写新LOB
                    processModifiedLob(sourceLob);
                    sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                }
                else if (compareLobResult == CompareLobResult.SOURCE_NEW) {
                    // 写新LOB
                    writeNewLob(sourceLob);
                    sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
                }
                else if (compareLobResult == CompareLobResult.NOT_USER_DATA) {
                    // 目标LOB不是迁移过来的LOB，跳过
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                }
                else {
                    if (deleteMoreLobInTarget) {
                        // 删除目标多余LOB
                        ObjectId lobId = (ObjectId) targetLob.get("Oid");
                        safeDeleteTargetLob(lobId);
                    }
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                }
                updateProgress();
                if (isTimeout() || !isRunning()) {
                    return false;
                }
            }

            while (sourceLob != null) {
                // 写新LOB
                writeNewLob(sourceLob);
                sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
                updateProgress();
                if (isTimeout() || !isRunning()) {
                    return false;
                }
            }

            // 删除目标多余LOB
            if (deleteMoreLobInTarget) {
                while (targetLob != null) {
                    ObjectId lobId = (ObjectId) targetLob.get("Oid");
                    safeDeleteTargetLob(lobId);
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                    updateProgress();
                    if (isTimeout() || !isRunning()) {
                        return false;
                    }
                }
            }

            if (failedLobNum > 0) {
                return false;
            }
            return true;
        }
    }

    private void safeDeleteTargetLob(ObjectId lobId) {
        try {
            deleteLob(targetCL, lobId);
        }
        catch (Exception e) {
            logger.error(
                    "failed to delete residue lob from target cl, sourceSite={}, targetSite={}, cl={}, lobId={}",
                    sourceSite, targetSite, targetCL.getFullName(), lobId, e);
        }
    }

    private void writeNewLob(BSONObject sourceLob) {
        ObjectId lobId = (ObjectId) sourceLob.get("Oid");
        try {
            if (writeLobInTargetCl(sourceLob)) {
                successLobNum++;
            }
            else {
                failedLobNum++;
            }
        }
        catch (Exception e) {
            logger.error(
                    "failed to write new lob to target cl, sourceSite={}, targetSite={}, cl={}, lobId={}",
                    sourceSite, targetSite, targetCL.getFullName(), lobId);
            failedLobNum++;
        }
    }

    private void processModifiedLob(BSONObject sourceLob) {
        ObjectId lobId = (ObjectId) sourceLob.get("Oid");
        try {
            // 删除目标LOB，写新LOB
            logger.info(
                    "the source lob is modify, delete and write new lob to target cl, sourceSite={}, targetSite={}, cl={}, lobId={}",
                    sourceSite, targetSite, sourceCL.getFullName(), lobId);
            deleteLob(targetCL, lobId);
            if (writeLobInTargetCl(sourceLob)) {
                successLobNum++;
            }
            else {
                failedLobNum++;
            }
        }
        catch (Exception e) {
            logger.error(
                    "failed to delete and write new lob to target cl, sourceSite={}, targetSite={}, cl={}, lobId={}",
                    sourceSite, targetSite, sourceCL.getFullName(), lobId, e);
            failedLobNum++;
        }
    }

    private boolean writeLobInTargetCl(BSONObject sourceLob) throws Exception {
        BSONObject userData = generateTargetLobUserData(sourceLob);
        ObjectId lobID = (ObjectId) sourceLob.get("Oid");
        DBLob tLob;
        try {
            tLob = targetCL.createLob(lobID, userData);
        }
        catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_FE.getErrorCode()) {
                logger.warn(
                        "failed to create lob, the target lob already is exist, targetSite={}, cl={}, lobId={}",
                        targetSite, targetCL.getFullName(), lobID);
                if (checkDataSame(lobID)) {
                    logger.info(
                            "check lob data is same, skipped write to target cl, sourceSite={}, targetSite={}, cl={}, lobId={}",
                            sourceSite, targetSite, targetCL.getFullName(), lobID);
                    return true;
                }
                else {
                    logger.warn(
                            "check lob data is diff, delete lob from target cl, sourceSite={}, targetSite={}, cl={}, lobId={}",
                            sourceSite, targetSite, targetCL.getFullName(), lobID);
                    deleteLob(targetCL, lobID);
                    tLob = targetCL.createLob(lobID, userData);
                }
            }
            else {
                throw e;
            }
        }

        MessageDigest sDigest = MessageDigest.getInstance("MD5");
        MessageDigest tDigest = MessageDigest.getInstance("MD5");
        DBLob sLob = null;
        try {
            sLob = sourceCL.openLob(lobID);
            byte[] buffer = new byte[BUFFER_SIZE];
            int bytesRead;

            while ((bytesRead = sLob.read(buffer)) != -1) {
                tLob.write(buffer, 0, bytesRead);
                sDigest.update(buffer, 0, bytesRead);
                tDigest.update(buffer, 0, bytesRead);
            }
            sLob.close();
            sLob = null;
            tLob.close();
        }
        catch (Exception e) {
            ScheduleCommonTools.closeResource(sLob, tLob);
            logger.error(
                    "write lob to cl failed, remove lob, sourceSite={}, targetSite={}, cl={}, lobId={}",
                    sourceSite, targetSite, targetCL.getFullName(), lobID, e);
            deleteLob(targetCL, lobID);
            return false;
        }
        String sMd5 = ScheduleCommonTools.bytesToHexStr(sDigest.digest());
        String tMd5 = ScheduleCommonTools.bytesToHexStr(tDigest.digest());
        if (!sMd5.equals(tMd5)) {
            logger.error(
                    "write lob success, but lob md5 is not same, delete lob, sourceSite={}, targetSite={}, cl={}, lobId={}, sourceMd5={}, targetMd5={}",
                    sourceSite, targetSite, targetCL.getFullName(), lobID, sMd5, tMd5);
            deleteLob(targetCL, lobID);
            return false;
        }
        transferLobSize += BsonUtils.getLongChecked(sourceLob, "Size");
        return true;
    }

    private BSONObject generateTargetLobUserData(BSONObject sourceLob) {
        BSONObject userData = new BasicBSONObject();
        userData.put("CreateTime", sourceLob.get("CreateTime"));
        userData.put("ModificationTime", sourceLob.get("ModificationTime"));
        return userData;
    }

    private void deleteLob(DBCollection cl, ObjectId lobId) throws ScheduleServerException {
        try {
            cl.removeLob(lobId);
        }
        catch (Exception e) {
            throw new ScheduleSystemException(
                    "failed to delete lob, cl=" + cl.getFullName() + ", lobId=" + lobId, e);
        }
    }

    private boolean checkDataSame(ObjectId lobID) throws ScheduleServerException {
        try (DBLob sLob = sourceCL.openLob(lobID); DBLob tLob = targetCL.openLob(lobID)) {
            MessageDigest sDigest = MessageDigest.getInstance("MD5");
            MessageDigest tDigest = MessageDigest.getInstance("MD5");

            byte[] sBuffer = new byte[BUFFER_SIZE];
            byte[] tBuffer = new byte[BUFFER_SIZE];
            int sLen;
            int tLen;

            while (true) {
                sLen = sLob.read(sBuffer);
                tLen = tLob.read(tBuffer);

                if (sLen == -1 && tLen == -1) {
                    break;
                }

                if (sLen != tLen) {
                    return false;
                }

                sDigest.update(sBuffer, 0, sLen);
                tDigest.update(tBuffer, 0, tLen);
            }

            String sMd5 = ScheduleCommonTools.bytesToHexStr(sDigest.digest());
            String tMd5 = ScheduleCommonTools.bytesToHexStr(tDigest.digest());

            return sMd5.equals(tMd5);
        }
        catch (Exception e) {
            throw new ScheduleSystemException("failed to check lob data same, lobId=" + lobID, e);
        }
    }

    public static CompareLobResult compareLobInfo(BSONObject sourceLob, BSONObject targetLob) {
        ObjectId sId = (ObjectId) sourceLob.get("Oid");
        ObjectId tId = (ObjectId) targetLob.get("Oid");
        if (!sId.equals(tId)) {
            BSONObject userData = (BSONObject) targetLob.get("UserData");
            if (userData == null) {
                // 没有自定义元数据，不是迁移过来的LOB，不管
                return CompareLobResult.NOT_USER_DATA;
            }

            BSONTimestamp sCreateTime = (BSONTimestamp) sourceLob.get("CreateTime");
            BSONTimestamp tCreateTime = (BSONTimestamp) userData.get("CreateTime");
            if (tCreateTime == null) {
                // 没有自定义元数据，不是迁移过来的LOB，不管
                return CompareLobResult.NOT_USER_DATA;
            }

            if (compareTime(sCreateTime, tCreateTime) <= 0) {
                // 源端新增，目标端缺失
                return CompareLobResult.SOURCE_NEW;
            }
            else {
                // 目标端残留，源端缺失
                return CompareLobResult.TARGET_RESIDUE;
            }
        }
        else {
            // Oid 相等，继续比较其他属性
            BSONObject userData = (BSONObject) targetLob.get("UserData");
            if (userData == null) {
                // 没有自定义元数据，直接视为被修改
                return CompareLobResult.LOB_MODIFY;
            }

            BSONTimestamp sCreateTime = (BSONTimestamp) sourceLob.get("CreateTime");
            BSONTimestamp tCreateTime = (BSONTimestamp) userData.get("CreateTime");
            if (tCreateTime == null) {
                // 没有自定义元数据，直接视为被修改
                return CompareLobResult.LOB_MODIFY;
            }
            if (!sCreateTime.equals(tCreateTime)) {
                // 创建时间不相等，LOB 视为被修改
                return CompareLobResult.LOB_MODIFY;
            }

            BSONTimestamp sModificationTime = (BSONTimestamp) sourceLob.get("ModificationTime");
            BSONTimestamp tModificationTime = (BSONTimestamp) userData.get("ModificationTime");
            if (tModificationTime == null) {
                // 没有自定义元数据，直接视为被修改
                return CompareLobResult.LOB_MODIFY;
            }
            if (!sModificationTime.equals(tModificationTime)) {
                // 修改时间不相等，LOB 视为被修改
                return CompareLobResult.LOB_MODIFY;
            }

            Long sSize = BsonUtils.getLongChecked(sourceLob, "Size");
            Long tSize = BsonUtils.getLongChecked(targetLob, "Size");
            if (!sSize.equals(tSize)) {
                return CompareLobResult.LOB_MODIFY;
            }
            return CompareLobResult.SAME;
        }
    }

    public static int compareTime(BSONTimestamp time1, BSONTimestamp time2) {
        if (time1.getTime() < time2.getTime()) {
            return -1;
        }
        else if (time1.getTime() > time2.getTime()) {
            return 1;
        }
        else {
            return Integer.compare(time1.getInc(), time2.getInc());
        }
    }

    private boolean isTimeout() {
        return maxExecTime > 0 && System.currentTimeMillis() - taskStartTime > maxExecTime;
    }
}
