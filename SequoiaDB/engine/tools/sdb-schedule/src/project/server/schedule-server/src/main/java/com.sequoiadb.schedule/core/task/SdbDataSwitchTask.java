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

   Source File Name = SdbDataSwitchTask.java

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.common.SdbHelper;
import com.sequoiadb.schedule.common.Version;
import com.sequoiadb.schedule.core.DataServiceWrapper;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.model.CollectionDataSwitchEvent;
import com.sequoiadb.schedule.model.CollectionSnapshotRecord;
import com.sequoiadb.schedule.model.SdbCLFullInfo;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class SdbDataSwitchTask extends TransferDataSwitchTaskBase {
    private static Logger logger = LoggerFactory.getLogger(SdbDataSwitchTask.class);
    private final Version MIN_SDB_VERSION = new Version(5, 8, 6);

    public SdbDataSwitchTask(ScheduleTaskMgr mgr, TaskEntity taskEntity,
            TransactionLock transactionLock) {
        super(mgr, taskEntity, transactionLock);
    }

    @Override
    public void checkSdbVersion() throws Exception {
        Version sourceSdbVer = getDataServiceVersion(getSourceSite());
        if (sourceSdbVer.compareTo(MIN_SDB_VERSION) < 0) {
            throw new ScheduleSystemException("source site is only supported in SequoiaDB version "
                    + MIN_SDB_VERSION + " or later");
        }

        Version targetSdbVer = getDataServiceVersion(getTargetSite());
        if (targetSdbVer.compareTo(MIN_SDB_VERSION) < 0) {
            throw new ScheduleSystemException("target site is only supported in SequoiaDB version "
                    + MIN_SDB_VERSION + " or later");
        }
    }

    @Override
    public void doTask() throws Exception {
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getSourceSite());
        Sequoiadb sdb = null;
        try {
            sdb = dataService.getConnection();
            doDataSwitch(sdb);
            // timeout
            if (isTimeout) {
                logger.warn(
                        "task have been interrupted because of timeout:taskId={},startTime={},now={},maxExecTime={}ms",
                        getTaskId(), taskStartTime, new Date(), maxExecTime);
                abortTaskAndAsyncRedo(getTaskId(),
                        ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_TIMEOUT, "timeout",
                        processClCount);
                return;
            }

            // canceled
            int flag = getTaskRunningFlag(getTaskId());
            if (ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_CANCEL == flag) {
                logger.info("task have been canceled:taskId={}", getTaskId());
                stopTaskAndAsyncRedo(getTaskId(), processClCount);
                return;
            }

            // finished
            logger.info("task have been finished:taskId={}", getTaskId());
            finishTaskAndAsyncRedo(getTaskId(), processClCount);
        }
        finally {
            if (sdb != null) {
                dataService.releaseConnection(sdb);
            }
        }
    }

    private void doDataSwitch(Sequoiadb sourceSdb) {
        Boolean partitionInterruption = BsonUtils.getBooleanOrElse(getContent(),
                FieldName.Schedule.FIELD_CONTENT_PARTITION_INTERRUPTION, true);
        if (partitionInterruption) {
            CLPartitionSortedRes sortedRes = getClAndSortByPartition(sourceSdb);
            for (String clFullName : sortedRes.getCommonAndMainClList()) {
                processClCount++;
                innerDataSwitchCl(sourceSdb, clFullName, new InterruptionFlag());
                if (checkTimeout() || !running) {
                    return;
                }
            }

            Map<String, List<String>> sortedSubClMap = sortedRes.getSortedSubClMap();
            for (Map.Entry<String, List<String>> entry : sortedSubClMap.entrySet()) {
                List<String> sortedSubClList = entry.getValue();
                InterruptionFlag interruptionFlag = new InterruptionFlag();
                for (String clFullName : sortedSubClList) {
                    processClCount++;
                    innerDataSwitchCl(sourceSdb, clFullName, interruptionFlag);
                    if (checkTimeout() || !running) {
                        return;
                    }
                    if (interruptionFlag.isInterrupted()) {
                        // 分区中断，当上一个分区的子表没有数据切换或者数据切换失败，则后续分区的子表不进行数据切换，跳过
                        logger.info(
                                "partition interruption occurred, skipped data switch other sub collections of the " +
                                        "main collection {}, currentSubCl: {}, sourceSite: {}, targetSite: {}, taskId: {}",
                                entry.getKey(), clFullName, getSourceSite(), getTargetSite(),
                                getTaskId());
                        break;
                    }
                }
            }
        }
        else {
            try (DBCursor cursor = getClCataSnapshot(sourceSdb)) {
                while (cursor.hasNext()) {
                    processClCount++;
                    BSONObject clCataInfo = cursor.getNext();
                    String clFullName = BsonUtils.getStringChecked(clCataInfo, "Name");
                    innerDataSwitchCl(sourceSdb, clFullName, new InterruptionFlag());
                    if (checkTimeout() || !running) {
                        return;
                    }
                }
            }
        }
    }

    private void innerDataSwitchCl(Sequoiadb sourceSdb, String clFullName,
            InterruptionFlag interruptionFlag) {
        TransactionLock clLock = null;
        try {
            clLock = createCollectionTransactionLock(clFullName);
            clLock.lock();
            dataSwitchCl(sourceSdb, clFullName, interruptionFlag);
        }
        catch (ScheduleServerException e) {
            interruptionFlag.markInterrupted();
            if (e.getError() == ScheduleServerError.TRANSACTION_LOCK_TIMEOUT) {
                logger.warn(
                        "do cl data switch failed because of get cl lock timeout, sourceSite: {}, targetSite: {}, cl: {}, taskId: {}",
                        getSourceSite(), getTargetSite(), clFullName, getTaskId());
            }
            else {
                logger.warn(
                        "do cl data switch failed, sourceSite: {}, targetSite: {}, cl: {}, taskId: {}",
                        getSourceSite(), getTargetSite(), clFullName, getTaskId(), e);
            }
        }
        catch (Exception e) {
            interruptionFlag.markInterrupted();
            logger.warn(
                    "do cl data switch failed, sourceSite: {}, targetSite: {}, cl: {}, taskId: {}",
                    getSourceSite(), getTargetSite(), clFullName, getTaskId(), e);
        }
        finally {
            if (clLock != null) {
                clLock.unlock();
            }
        }
    }

    private void dataSwitchCl(Sequoiadb sourceSdb, String clFullName,
            InterruptionFlag interruptionFlag) throws Exception {
        BSONObject clCataInfo = SdbHelper.getCataSnapshotByClName(sourceSdb, clFullName);
        if (clCataInfo == null) {
            logger.warn(
                    "the cl cata info is null, skip data switch. sourceSite: {}, clName: {}, taskId: {}",
                    getSourceSite(), clFullName, getTaskId());
            return;
        }
        boolean isMapDs = clCataInfo.containsField("DataSourceID");
        if (isMapDs) {
            logger.debug("cl is already map to data source, skip data switch. clName: {}",
                    clFullName);
            return;
        }

        Boolean isMainCL = BsonUtils.getBooleanOrElse(clCataInfo, "IsMainCL", false);
        // 主表不需要数据切换
        if (!isMainCL) {
            DataSwitchRes dataSwitchRes = commonDataSwitchCl(sourceSdb, clCataInfo);
            saveDataSwitchProgress(clFullName, dataSwitchRes);
            if (!dataSwitchRes.isDataSwitched()) {
                interruptionFlag.markInterrupted();
            }
        }
    }

    private void saveDataSwitchProgress(String clName, DataSwitchRes dataSwitchRes) {
        try {
            ScheduleServer.getInstance().saveTaskDataSwitchProgress(getTaskId(), clName,
                    dataSwitchRes);
        }
        catch (Exception e) {
            logger.warn(
                    "save data switch progress failed, taskId: {}, clName: {}, canDataSwitch: {}, isDataSwitched: {}",
                    getTaskId(), clName, dataSwitchRes.isCanDataSwitch(),
                    dataSwitchRes.isDataSwitched(), e);
        }
    }

    private DataSwitchRes commonDataSwitchCl(Sequoiadb sourceSdb, BSONObject clCataInfo)
            throws Exception {
        if (!canSchedule(clCataInfo)) {
            return new DataSwitchRes(false, false);
        }
        String clFullName = BsonUtils.getStringChecked(clCataInfo, "Name");
        String mainCLName = BsonUtils.getString(clCataInfo, "MainCLName");
        if (mainCLName != null && !mainCLName.isEmpty()) {
            boolean inTargetSite = isClExistInTargetSite(mainCLName);
            if (!inTargetSite) {
                logger.info(
                        "the sub cl's main cl is not exist in target site, may not be transfer, skip data switch, subCl: {}, mainCl: {}",
                        clFullName, mainCLName);
                return new DataSwitchRes(false, false);
            }
        }

        if (!isClExistInTargetSite(clFullName)) {
            logger.info(
                    "the cl is not exist in target site, may not be transfer, skip data switch, cl: {}",
                    clFullName);
            return new DataSwitchRes(false, false);
        }

        CollectionSnapshotRecord lastCollectionSnapshotRecord = ScheduleServer.getInstance()
                .getLastCollectionSnapshotRecord(getSourceSite(), clFullName);
        // 没有快照记录的情况下，说明集合没有发生过迁移，不能进行数据切换
        if (lastCollectionSnapshotRecord == null) {
            return new DataSwitchRes(false, false);
        }

        // 快照无效的情况下，说明还没有迁移完成，不能进行数据切换
        if (!lastCollectionSnapshotRecord.isRecordSnapshotEffective()
                || !lastCollectionSnapshotRecord.isLobSnapshotEffective()) {
            return new DataSwitchRes(false, false);
        }

        try {
            boolean clMetaConsistent = checkClMetaConsistent(clCataInfo, clFullName);
            if (!clMetaConsistent) {
                return new DataSwitchRes(false, false);
            }
        }
        catch (Exception e) {
            throw new Exception(
                    "check collection meta info consistent between source site and target site failed, cl: "
                            + clFullName,
                    e);
        }

        try {
            boolean indexConsistent = checkIndexConsistent(sourceSdb, clFullName);
            if (!indexConsistent) {
                return new DataSwitchRes(false, false);
            }
        }
        catch (Exception e) {
            throw new Exception(
                    "check collection index info consistent between source site and target site failed, cl: "
                            + clFullName,
                    e);
        }

        try {
            boolean isAutoIncFieldConsistent = checkAutoIncFieldConsistent(clCataInfo, clFullName);
            if (!isAutoIncFieldConsistent) {
                return new DataSwitchRes(false, false);
            }
        }
        catch (Exception e) {
            throw new Exception(
                    "check autoIncField consistent between source site and target site failed, cl: "
                            + clFullName,
                    e);
        }

        // 隐藏开关，默认关闭，用于是否检查数据一致性
        Boolean checkDataConsistent = BsonUtils.getBooleanOrElse(getContent(),
                FieldName.Schedule.FIELD_CONTENT_CHECK_DATA_CONSISTENT, false);

        if (checkDataConsistent) {
            try {
                boolean isDataConsistent = checkDataConsistent(sourceSdb, clFullName);
                if (!isDataConsistent) {
                    // 数据不一致，跳过清理
                    return new DataSwitchRes(false, false);
                }
            }
            catch (Exception e) {
                throw new Exception(
                        "check data consistent between source site and target site failed, cl: "
                                + clFullName,
                        e);
            }
        }

        if (!notDataWrite(sourceSdb, clFullName)) {
            return new DataSwitchRes(false, false);
        }

        if (checkDataConsistent) {
            SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clFullName);
            DBCollection collection = sourceSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName());
            try {
                boolean isDataConsistent = checkDataConsistent(sourceSdb, clFullName);
                if (!isDataConsistent) {
                    logger.info(
                            "the data is not consistent between source site and target site, skip data switch, cl: {}",
                            clFullName);
                    SdbHelper.unSetClRepairCheck(collection);
                    return new DataSwitchRes(false, false);
                }
            }
            catch (Exception e) {
                SdbHelper.unSetClRepairCheck(collection);
                throw new Exception(
                        "check data consistent between source site and target site failed, cl: "
                                + clFullName,
                        e);
            }
        }

        dropAndMapToDs(sourceSdb, clFullName, clCataInfo);
        return new DataSwitchRes(true, true);
    }

    private boolean checkIndexConsistent(Sequoiadb sourceSdb, String clFullName) throws Exception {
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clFullName);
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSdb = null;
        try {
            targetSdb = dataService.getConnection();
            DBCollection sourceCL = sourceSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName());
            DBCollection targetCL = targetSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName());

            try (DBCursor cursor = sourceCL.getIndexes()) {
                while (cursor.hasNext()) {
                    BSONObject indexObj = cursor.getNext();
                    BSONObject indexDef = BsonUtils.getBSONChecked(indexObj, "IndexDef");
                    String name = BsonUtils.getStringChecked(indexDef, "name");
                    if (targetCL.isIndexExist(name)) {
                        BSONObject sourceKey = BsonUtils.getBSONChecked(indexDef, "key");
                        BSONObject targetIndexInfo = targetCL.getIndexInfo(name);
                        BSONObject targetIndexDef = BsonUtils.getBSONChecked(targetIndexInfo,
                                "IndexDef");
                        BSONObject targetKey = BsonUtils.getBSONChecked(targetIndexDef, "key");
                        if (!sourceKey.equals(targetKey)) {
                            return false;
                        }
                    }
                    else {
                        return false;
                    }
                }
            }
            return true;
        }
        finally {
            if (targetSdb != null) {
                dataService.releaseConnection(targetSdb);
            }
        }
    }

    private boolean checkClMetaConsistent(BSONObject sourceCataInfo, String clFullName)
            throws Exception {
        BSONObject targetCataInfo = null;
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSdb = null;
        try {
            targetSdb = dataService.getConnection();
            targetCataInfo = SdbHelper.getCataSnapshotByClName(targetSdb, clFullName);
        }
        finally {
            if (targetSdb != null) {
                dataService.releaseConnection(targetSdb);
            }
        }

        BSONObject newOption = compareAndReturnNewOption(sourceCataInfo, targetCataInfo,
                clFullName);
        if (newOption != null) {
            logger.info("cl meta inconsistent, cl={}, newOption={}", clFullName, newOption);
            return false;
        }
        return true;
    }

    private boolean checkAutoIncFieldConsistent(BSONObject clCataInfo, String clName)
            throws Exception {
        Set<String> sourceAutoIncFields = getAutoIncFields(clCataInfo);
        if (sourceAutoIncFields == null || sourceAutoIncFields.isEmpty()) {
            return true;
        }

        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSdb = null;
        Set<String> targetAutoIncFields = null;
        try {
            targetSdb = dataService.getConnection();
            targetAutoIncFields = getAutoIncFields(targetSdb, clName, getTargetSite());
        }
        catch (Exception e) {
            throw new ScheduleSystemException(
                    "check cl AutoIncrement between source site and target site failed, cl: "
                            + clName,
                    e);
        }
        finally {
            if (targetSdb != null) {
                dataService.releaseConnection(targetSdb);
            }
        }

        if (targetAutoIncFields == null || targetAutoIncFields.isEmpty()) {
            return false;
        }

        if (targetAutoIncFields.size() < sourceAutoIncFields.size()) {
            return false;
        }

        for (String sourceAutoIncField : sourceAutoIncFields) {
            if (!targetAutoIncFields.contains(sourceAutoIncField)) {
                return false;
            }
        }

        return true;
    }

    private void dropAndMapToDs(Sequoiadb sourceSdb, String clFullName, BSONObject clCataInfo)
            throws Exception {
        String mainCLName = BsonUtils.getString(clCataInfo, "MainCLName");
        boolean isSubCl = mainCLName != null && !mainCLName.isEmpty();
        BSONObject sAttachInfo = null;
        if (isSubCl) {
            sAttachInfo = SdbHelper.getAttachInfo(sourceSdb, mainCLName, clFullName);
            BSONObject tAttachInfo = getAttachInfoFromTarget(mainCLName, clFullName);
            if (!attachInfoIsSame(sAttachInfo, tAttachInfo)) {
                throw new Exception(
                        "the attach info is different between source site and target site, cannot data switch the sub cl: "
                                + clFullName + ", source attachInfo: " + sAttachInfo
                                + ", target attachInfo: " + tAttachInfo);
            }
        }

        String rename = clFullName + "_data_switch_bak_" + System.currentTimeMillis();

        CollectionDataSwitchEvent event = new CollectionDataSwitchEvent(clFullName, getSourceSite(),
                getTargetSite(), getDatasourceName(), clCataInfo, sAttachInfo, rename,
                ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHING);
        ScheduleServer.getInstance().insertDataSwitchEvent(event);

        if (isSubCl) {
            logger.info("detach sub cl [{}] from main cl [{}], site: {}", clFullName, mainCLName,
                    getSourceSite());
            SdbHelper.detachCl(sourceSdb, mainCLName, clFullName);
        }

        logger.info("rename cl [{}] to [{}], site: {}", clFullName, rename, getSourceSite());
        SdbHelper.renameCl(sourceSdb, clFullName, rename);

        logger.info("create cl and map to data source, cl {}, site: {}, datasource: {}", clFullName,
                getSourceSite(), getDatasourceName());
        createCLMapToDS(sourceSdb, clFullName, getDatasourceName());

        if (isSubCl) {
            logger.info("attach sub cl [{}] to main cl [{}], site: {}, attachInfo: {}", clFullName,
                    mainCLName, getSourceSite(), sAttachInfo);
            SdbHelper.attachCl(sourceSdb, mainCLName, clFullName,
                    (BSONObject) sAttachInfo.get("LowBound"),
                    (BSONObject) sAttachInfo.get("UpBound"));
        }

        changeDataSwitchingEventStatusSwitched(event);
    }

    private void changeDataSwitchingEventStatusSwitched(CollectionDataSwitchEvent event) {
        try {
            BSONObject newValue = new BasicBSONObject(
                    FieldName.CollectionDataSwitchEvent.FIELD_STATUS,
                    ScheduleDefine.CollectionDataSwitchStatus.DATA_SWITCHED);
            newValue.put(FieldName.CollectionDataSwitchEvent.FIELD_DATA_SWITCHED_TIME,
                    System.currentTimeMillis());

            ScheduleServer.getInstance().updateCollectionDataSwitchEvent(event, newValue);
        }
        catch (Exception e) {
            logger.warn(
                    "update collection data switch event status to 'data_switched' failed, event: {}",
                    event.toBSONObject(), e);
        }
    }

    private BSONObject getAttachInfoFromTarget(String mainCLName, String clName) throws Exception {
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb sdb = null;
        try {
            sdb = dataService.getConnection();
            return SdbHelper.getAttachInfo(sdb, mainCLName, clName);
        }
        finally {
            if (sdb != null) {
                dataService.releaseConnection(sdb);
            }
        }
    }

    public static void createCLMapToDS(Sequoiadb sourceSdb, String clName, String datasource) {
        BSONObject clOptions = new BasicBSONObject();
        clOptions.put("DataSource", datasource);
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clName);
        sourceSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                .createCollection(sdbCLFullInfo.getClName(), clOptions);
    }

    // 比对两边集合数据是否一致
    private boolean checkDataConsistent(Sequoiadb sourceSdb, String clName) throws Exception {
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSdb = null;
        try {
            targetSdb = dataService.getConnection();
            SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clName);
            DBCollection sCl = sourceSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName());
            DBCollection tCl = targetSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName());

            BSONObject one = sCl.queryOne();
            if (one != null && !isRecordConsistent(sCl, tCl)) {
                return false;
            }

            if (isLobConsistent(sCl, tCl)) {
                return true;
            }
            return false;
        }
        catch (Exception e) {
            throw new ScheduleSystemException(
                    "compare data between source site and target site failed, cl: " + clName, e);
        }
        finally {
            if (targetSdb != null) {
                dataService.releaseConnection(targetSdb);
            }
        }
    }

    private boolean isRecordConsistent(DBCollection sCl, DBCollection tCl) {
        long sCount = sCl.getCount();
        long tCount = tCl.getCount();
        if (sCount > tCount) {
            return false;
        }

        ObjectId lastRecordId = null;
        try (DBCursor cursor = sCl.query(null, new BasicBSONObject("_id", 1),
                new BasicBSONObject("_id", -1), null, 0, 1)) {
            if (cursor.hasNext()) {
                BSONObject lastRecord = cursor.getNext();
                lastRecordId = (ObjectId) lastRecord.get("_id");
            }
        }

        if (lastRecordId == null) {
            return true;
        }

        long sNum = sCl.getCount(new BasicBSONObject("_id", lastRecordId));
        long tNum = tCl.getCount(new BasicBSONObject("_id", lastRecordId));
        return sNum == tNum;
    }

    private boolean isLobConsistent(DBCollection sCl, DBCollection tCL) throws Exception {
        BSONObject sSort = new BasicBSONObject("CreateTime", 1);
        sSort.put("Oid", 1);

        BSONObject tSort = new BasicBSONObject("UserData.CreateTime", 1);
        tSort.put("Oid", 1);

        try (DBCursor sourceCursor = sCl.listLobs(null, null, sSort, null, 0, -1);
                DBCursor targetCursor = tCL.listLobs(null, null, tSort, null, 0, -1);) {

            BSONObject sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
            BSONObject targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;

            while (sourceLob != null && targetLob != null) {
                SdbCLTransferHelper.CompareLobResult compareLobResult = SdbCLTransferHelper
                        .compareLobInfo(sourceLob, targetLob);
                if (compareLobResult == SdbCLTransferHelper.CompareLobResult.SAME) {
                    sourceLob = sourceCursor.hasNext() ? sourceCursor.getNext() : null;
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                }
                else if (compareLobResult == SdbCLTransferHelper.CompareLobResult.NOT_USER_DATA) {
                    // 目标LOB不是迁移过来的LOB，跳过
                    targetLob = targetCursor.hasNext() ? targetCursor.getNext() : null;
                }
                else {
                    return false;
                }
            }

            return sourceLob == null;
        }
    }

    private boolean isClExistInTargetSite(String clName) throws Exception {
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb sdb = null;
        try {
            sdb = dataService.getConnection();
            return SdbHelper.isClExist(sdb, clName);
        }
        finally {
            if (sdb != null) {
                dataService.releaseConnection(sdb);
            }
        }
    }

    @Override
    public String getName() {
        return "SDB_TASK_DATA_SWITCH";
    }

    @Override
    public int getTaskType() {
        return ScheduleDefine.TaskType.DATA_SWITCH;
    }

    public static class DataSwitchRes {
        private boolean canDataSwitch;
        private boolean dataSwitched;

        public DataSwitchRes(boolean canDataSwitch, boolean dataSwitched) {
            this.canDataSwitch = canDataSwitch;
            this.dataSwitched = dataSwitched;
        }

        public boolean isCanDataSwitch() {
            return canDataSwitch;
        }

        public boolean isDataSwitched() {
            return dataSwitched;
        }
    }

}
