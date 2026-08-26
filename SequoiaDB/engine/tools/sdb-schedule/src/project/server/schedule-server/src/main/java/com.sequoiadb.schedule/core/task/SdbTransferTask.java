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

   Source File Name = SdbTransferTask.java

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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.schedule.model.CollectionSnapshotRecord;
import com.sequoiadb.schedule.model.SdbCLFullInfo;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class SdbTransferTask extends TransferDataSwitchTaskBase {

    private final Version MIN_TARGET_SDB_VERSION = new Version(5, 8, 6);

    private static Logger logger = LoggerFactory.getLogger(SdbTransferTask.class);
    private SnapshotSkipStrategy recordSkipStrategy;
    private SnapshotSkipStrategy lobSkipStrategy;

    public SdbTransferTask(ScheduleTaskMgr mgr, TaskEntity taskEntity,
            TransactionLock transactionLock) {
        super(mgr, taskEntity, transactionLock);
        recordSkipStrategy = new SnapshotSkipStrategy() {
            @Override
            public boolean isSnapshotEffective(CollectionSnapshotRecord record) {
                return record.isRecordSnapshotEffective();
            }

            @Override
            public boolean compareSnapshot(BasicBSONList oldSnapshots, BasicBSONList newSnapshots) {
                return compareRecordSnapshot(oldSnapshots, newSnapshots);
            }
        };
        lobSkipStrategy = new SnapshotSkipStrategy() {
            @Override
            public boolean isSnapshotEffective(CollectionSnapshotRecord record) {
                return record.isLobSnapshotEffective();
            }

            @Override
            public boolean compareSnapshot(BasicBSONList oldSnapshots, BasicBSONList newSnapshots) {
                return compareLobSnapshot(oldSnapshots, newSnapshots);
            }
        };
    }

    @Override
    public void checkSdbVersion() throws Exception {
        Version ver = getDataServiceVersion(getTargetSite());
        if (ver.compareTo(MIN_TARGET_SDB_VERSION) < 0) {
            throw new ScheduleSystemException(
                    "data source site is only supported in SequoiaDB version "
                            + MIN_TARGET_SDB_VERSION + " or later");
        }
    }

    @Override
    public void doTask() throws Exception {
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getSourceSite());
        Sequoiadb sdb = null;
        try {
            sdb = dataService.getConnection();
            doTransfer(sdb);
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

    private void doTransfer(Sequoiadb sourceSdb) {
        Boolean partitionInterruption = BsonUtils.getBooleanOrElse(getContent(),
                FieldName.Schedule.FIELD_CONTENT_PARTITION_INTERRUPTION, true);
        if (partitionInterruption) {
            CLPartitionSortedRes sortedRes = getClAndSortByPartition(sourceSdb);
            for (String clFullName : sortedRes.getCommonAndMainClList()) {
                processClCount++;
                transferCl(sourceSdb, clFullName, new InterruptionFlag());
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
                    transferCl(sourceSdb, clFullName, interruptionFlag);
                    if (checkTimeout() || !running) {
                        return;
                    }
                    // 分区中断，当上一个分区的子表不满足迁移条件或者迁移失败，则后续分区的子表不迁移，跳过
                    if (interruptionFlag.isInterrupted()) {
                        logger.info(
                                "partition interruption! skipped transfer other sub collections of the main collection [{}], currentSubCl={}, sourceSite={}, targetSite={}, taskId={}",
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
                    transferCl(sourceSdb, clFullName, new InterruptionFlag());
                    if (checkTimeout() || !running) {
                        return;
                    }
                }
            }
        }
    }

    private void transferCl(Sequoiadb sourceSdb, String clFullName,
            InterruptionFlag interruptionFlag) {
        TransactionLock clLock = null;
        try {
            clLock = createCollectionTransactionLock(clFullName);
            clLock.lock();
            innerTransferCl(sourceSdb, clFullName, interruptionFlag);
        }
        catch (ScheduleServerException e) {
            interruptionFlag.markInterrupted();
            if (e.getError() == ScheduleServerError.TRANSACTION_LOCK_TIMEOUT) {
                logger.warn(
                        "do transfer cl failed because of get cl lock timeout, sourceSite={}, targetSite={}, cl={}, taskId={}",
                        getSourceSite(), getTargetSite(), clFullName, getTaskId());
            }
            else {
                logger.warn(
                        "do transfer cl failed, sourceSite={}, targetSite={}, cl={}, taskId={}",
                        getSourceSite(), getTargetSite(), clFullName, getTaskId(), e);
            }
        }
        catch (Exception e) {
            interruptionFlag.markInterrupted();
            logger.warn("do transfer cl failed, sourceSite={}, targetSite={}, cl={}, taskId={}",
                    getSourceSite(), getTargetSite(), clFullName, getTaskId(), e);
        }
        finally {
            if (clLock != null) {
                clLock.unlock();
            }
        }
    }

    private void innerTransferCl(Sequoiadb sourceSdb, String clFullName,
            InterruptionFlag interruptionFlag) throws Exception {
        BSONObject clCataInfo = SdbHelper.getCataSnapshotByClName(sourceSdb, clFullName);
        if (clCataInfo == null) {
            logger.warn(
                    "cl not exist in source site, skip transfer. sourceSite={}, targetSite={}, clName={}, taskId={}",
                    getSourceSite(), getTargetSite(), clFullName, getTaskId());
            return;
        }
        boolean isMapDs = clCataInfo.containsField("DataSourceID");
        if (isMapDs) {
            logger.debug(
                    "cl is already map to data source, skip transfer. sourceSite={}, targetSite={}, clName={}, taskId={}",
                    getSourceSite(), getTargetSite(), clFullName, getTaskId());
            return;
        }

        Boolean isMainCL = BsonUtils.getBooleanOrElse(clCataInfo, "IsMainCL", false);
        if (isMainCL) {
            // transfer main cl
            _transferMainCl(sourceSdb, clCataInfo);
        }
        else {
            // transfer sub cl
            commonTransferCl(sourceSdb, clCataInfo, interruptionFlag);
        }
    }

    private void correctCLTransferStatus(BSONObject clCataInfo) throws Exception {
        String csCLName = BsonUtils.getStringChecked(clCataInfo, "Name");
        DataServiceWrapper targetSiteDataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        boolean isCLExist = false;
        Sequoiadb targetSiteSdb = null;
        try {
            targetSiteSdb = targetSiteDataService.getConnection();
            isCLExist = SdbHelper.isClExist(targetSiteSdb, csCLName);
        }
        finally {
            if (targetSiteSdb != null) {
                targetSiteDataService.releaseConnection(targetSiteSdb);
            }
        }

        if (isCLExist) {
            return;
        }

        // 目标站点不存在集合，清空该集合的迁移状态，避免因目标站点上的集合被人为删除导致迁移状态残留
        ScheduleServer.getInstance().cleanTransferStatus(getSourceSite(), getTargetSite(), csCLName);
    }

    private void createSubClInTargetSite(Sequoiadb sourceSdb, BSONObject clCataInfo)
            throws Exception {
        String csCLName = BsonUtils.getStringChecked(clCataInfo, "Name");
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(csCLName);

        DataServiceWrapper targetSiteDataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSiteSdb = null;
        try {
            targetSiteSdb = targetSiteDataService.getConnection();
            ensureCollectionSpaceAndCollection(sourceSdb, targetSiteSdb, sdbCLFullInfo, clCataInfo);
        }
        finally {
            if (targetSiteSdb != null) {
                targetSiteDataService.releaseConnection(targetSiteSdb);
            }
        }
    }

    private void ensureCollectionSpaceAndCollection(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb,
            SdbCLFullInfo sdbCLFullInfo, BSONObject clCataInfo) throws Exception {
        String csName = sdbCLFullInfo.getCsName();
        String clName = sdbCLFullInfo.getClName();

        boolean csExist = targetSiteSdb.isCollectionSpaceExist(csName);
        if (!csExist) {
            BSONObject csSnapshot = SdbHelper.getCSSnapshot(sourceSdb, csName);
            if (csSnapshot == null) {
                throw new IllegalArgumentException("cs not exist in source site, source site="
                        + getSourceSite() + ", csName=" + csName);
            }

            Integer pageSize = BsonUtils.getInteger(csSnapshot, "PageSize");
            Integer lobPageSize = BsonUtils.getInteger(csSnapshot, "LobPageSize");
            BSONObject options = new BasicBSONObject();
            if (pageSize != null) {
                options.put("PageSize", pageSize);
            }
            if (lobPageSize != null) {
                options.put("LobPageSize", lobPageSize);
            }

            String dataDomain = BsonUtils.getString(getContent(),
                    FieldName.Schedule.FIELD_CONTENT_DATA_DOMAIN);
            if (dataDomain != null && !dataDomain.isEmpty()) {
                options.put("Domain", dataDomain);
            }

            logger.info(
                    "create collection space in target site, targetSite={}, csName={}, options={}",
                    getTargetSite(), csName, options);
            targetSiteSdb.createCollectionSpace(csName, options);
        }

        CollectionSpace cs = targetSiteSdb.getCollectionSpace(csName);
        if (cs.isCollectionExist(clName)) {
            if (needReTransfer(sourceSdb, targetSiteSdb, clCataInfo)) {
                logger.info(
                        "sub cl attach range is diff, re transfer cl, sourceSite={}, targetSite={}, clName={}",
                        getSourceSite(), getTargetSite(), sdbCLFullInfo.getFullName());
                ScheduleServer.getInstance().cleanTransferStatus(getSourceSite(), getTargetSite(),
                        sdbCLFullInfo.getFullName());
                cs.dropCollection(clName);
                createAndAttachCl(sourceSdb, targetSiteSdb, sdbCLFullInfo, clCataInfo, cs, csName,
                        clName);
            }
            else {
                updateClMeta(sourceSdb, targetSiteSdb, sdbCLFullInfo);
                addOrUpdateAutoInc(sourceSdb, targetSiteSdb, sdbCLFullInfo);
                // 如果集合已经存在，仍然确保索引同源站点一致
                createIndexes(sourceSdb, targetSiteSdb, sdbCLFullInfo);
                attachCL(sourceSdb, targetSiteSdb, clCataInfo);
            }
        }
        else {
            createAndAttachCl(sourceSdb, targetSiteSdb, sdbCLFullInfo, clCataInfo, cs, csName,
                    clName);
        }
    }

    // 检查是否需要重新迁移集合，条件是源端和目标端子表的分区范围不一致
    private boolean needReTransfer(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb,
            BSONObject clCataInfo) {
        if (BsonUtils.getBooleanOrElse(clCataInfo, "IsMainCL", false)) {
            return false;
        }

        String sMainCLName = BsonUtils.getString(clCataInfo, "MainCLName");
        if (sMainCLName == null || sMainCLName.isEmpty()) {
            return false;
        }

        String subCLName = BsonUtils.getStringChecked(clCataInfo, "Name");
        BSONObject tClCataInfo = SdbHelper.getCataSnapshotByClName(targetSiteSdb, subCLName);
        if (tClCataInfo == null) {
            throw new IllegalArgumentException("sub cl not exist in target site, target site="
                    + getTargetSite() + ", subClName=" + subCLName);
        }

        if (!tClCataInfo.containsField("MainCLName")) {
            // 还没有被attach
            return false;
        }
        String tMainCLName = BsonUtils.getString(tClCataInfo, "MainCLName");

        BSONObject sAttachInfo = SdbHelper.getAttachInfo(sourceSdb, sMainCLName, subCLName);
        if (sAttachInfo == null) {
            throw new IllegalArgumentException(
                    "sub cl attach info not exist in source site, source site=" + getSourceSite()
                            + ", mainClName=" + sMainCLName + ", subClName=" + subCLName);
        }

        BSONObject tAttachInfo = SdbHelper.getAttachInfo(targetSiteSdb, tMainCLName, subCLName);
        if (tAttachInfo == null) {
            throw new IllegalArgumentException(
                    "sub cl attach info not exist in target site, target site=" + getTargetSite()
                            + ", mainClName=" + tMainCLName + ", subClName=" + subCLName);
        }

        return !attachInfoIsSame(sAttachInfo, tAttachInfo);
    }

    private void createAndAttachCl(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb,
            SdbCLFullInfo sdbCLFullInfo, BSONObject clCataInfo, CollectionSpace cs, String csName,
            String clName) throws ScheduleServerException {
        BSONObject clOptions = generateCreateClOptions(sourceSdb, clCataInfo, sdbCLFullInfo);
        logger.info("create collection in target site, targetSite={}, clName={}.{}, options={}",
                getTargetSite(), csName, clName, clOptions);
        cs.createCollection(clName, clOptions);
        createIndexes(sourceSdb, targetSiteSdb, sdbCLFullInfo);
        attachCL(sourceSdb, targetSiteSdb, clCataInfo);
    }

    private void updateClMeta(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb,
            SdbCLFullInfo sdbCLFullInfo) throws ScheduleSystemException {
        String clFullName = sdbCLFullInfo.getFullName();
        BSONObject sourceCataInfo = SdbHelper.getCataSnapshotByClName(sourceSdb, clFullName);
        BSONObject targetCataInfo = SdbHelper.getCataSnapshotByClName(targetSiteSdb, clFullName);
        BSONObject newOption = compareAndReturnNewOption(sourceCataInfo, targetCataInfo, clFullName);
        if (newOption == null) {
            return;
        }

        logger.info("the cl meta is diff, update meta, targetSite={}, clName={}, newOptions={}",
                getTargetSite(), sdbCLFullInfo.getFullName(), newOption);
        targetSiteSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                .getCollection(sdbCLFullInfo.getClName()).alterCollection(newOption);
    }


    private void attachCL(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb, BSONObject clCataInfo)
            throws ScheduleServerException {
        if (BsonUtils.getBooleanOrElse(clCataInfo, "IsMainCL", false)) {
            return;
        }

        String mainCLName = BsonUtils.getString(clCataInfo, "MainCLName");
        if (mainCLName == null || mainCLName.isEmpty()) {
            return;
        }
        String subCLName = BsonUtils.getStringChecked(clCataInfo, "Name");
        BSONObject targetClCataInfo = SdbHelper.getCataSnapshotByClName(targetSiteSdb, subCLName);
        if (targetClCataInfo == null) {
            throw new IllegalArgumentException("sub cl not exist in target site, target site="
                    + getTargetSite() + ", subClName=" + subCLName);
        }
        if (targetClCataInfo.containsField("MainCLName")) {
            // 已经被attach过了
            return;
        }

        BSONObject attachInfo = SdbHelper.getAttachInfo(sourceSdb, mainCLName, subCLName);
        if (attachInfo == null) {
            throw new IllegalArgumentException(
                    "sub cl attach info not exist in source site, source site=" + getSourceSite()
                            + ", mainClName=" + mainCLName + ", subClName=" + subCLName);
        }

        logger.info("attach sub cl [{}] to main cl [{}], site={}, attachInfo={}", subCLName,
                mainCLName, getTargetSite(), attachInfo);
        SdbHelper.attachCl(targetSiteSdb, mainCLName, subCLName,
                (BSONObject) attachInfo.get("LowBound"), (BSONObject) attachInfo.get("UpBound"));
    }

    private void createIndexes(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb,
            SdbCLFullInfo sdbCLFullInfo) {
        DBCollection sourceCL = sourceSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                .getCollection(sdbCLFullInfo.getClName());
        DBCollection targetCL = targetSiteSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
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
                    if (sourceKey.equals(targetKey)) {
                        continue;
                    }
                    else {
                        logger.info(
                                "the same name index is diff, delete target index, targetSite={}, clName={}, indexName={}",
                                getTargetSite(), sdbCLFullInfo.getFullName(), name);
                        try {
                            targetCL.dropIndex(name);
                        }
                        catch (BaseException e) {
                            if (e.getErrorCode() != SDBError.SDB_IXM_NOTEXIST.getErrorCode()) {
                                throw e;
                            }
                        }
                    }
                }

                BSONObject key = BsonUtils.getBSONChecked(indexDef, "key");
                boolean unique = BsonUtils.getBooleanOrElse(indexDef, "unique", false);
                boolean enforced = BsonUtils.getBooleanOrElse(indexDef, "enforced", false);
                boolean notNull = BsonUtils.getBooleanOrElse(indexDef, "NotNull", false);
                boolean notArray = BsonUtils.getBooleanOrElse(indexDef, "NotArray", false);

                BSONObject indexAttr = new BasicBSONObject();
                indexAttr.put("Unique", unique);
                indexAttr.put("Enforced", enforced);
                indexAttr.put("NotNull", notNull);
                indexAttr.put("NotArray", notArray);
                logger.info(
                        "create index, targetSite={}, clName={}, indexName={}, indexDef={}, indexAttr={}",
                        getTargetSite(), sdbCLFullInfo.getFullName(), name, key, indexAttr);
                try {
                    targetCL.createIndex(name, key, indexAttr);
                }
                catch (BaseException e) {
                    if (e.getErrorCode() != SDBError.SDB_IXM_EXIST.getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_IXM_REDEF.getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE
                                    .getErrorCode()) {
                        throw e;
                    }
                }
            }
        }
    }

    private void _transferMainCl(Sequoiadb sourceSdb, BSONObject mainClCataInfo) throws Exception {
        String csCLName = BsonUtils.getStringChecked(mainClCataInfo, "Name");
        SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(csCLName);

        DataServiceWrapper targetSiteDataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSiteSdb = null;
        try {
            targetSiteSdb = targetSiteDataService.getConnection();
            ensureCollectionSpaceAndCollection(sourceSdb, targetSiteSdb, sdbCLFullInfo,
                    mainClCataInfo);
        }
        finally {
            if (targetSiteSdb != null) {
                targetSiteDataService.releaseConnection(targetSiteSdb);
            }
        }
    }

    private void commonTransferCl(Sequoiadb sourceSdb, BSONObject clCataInfo,
            InterruptionFlag interruptionFlag) throws Exception {
        if (!canSchedule(clCataInfo)) {
            interruptionFlag.markInterrupted();
            return;
        }

        String mainCLName = BsonUtils.getString(clCataInfo, "MainCLName");
        if (mainCLName != null && !mainCLName.isEmpty()) {
            transferMainCl(sourceSdb, mainCLName);
        }

        correctCLTransferStatus(clCataInfo);
        createSubClInTargetSite(sourceSdb, clCataInfo);

        String clFullName = BsonUtils.getStringChecked(clCataInfo, "Name");
        SdbCLFullInfo clFullInfo = new SdbCLFullInfo(clFullName);
        DBCollection sourceCL = sourceSdb.getCollectionSpace(clFullInfo.getCsName())
                .getCollection(clFullInfo.getClName());
        DataServiceWrapper targetSiteDataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSiteSdb = null;
        try {
            targetSiteSdb = targetSiteDataService.getConnection();
            DBCollection targetCL = targetSiteSdb.getCollectionSpace(clFullInfo.getCsName())
                    .getCollection(clFullInfo.getClName());
            Boolean deleteMoreLobInTarget = BsonUtils.getBooleanOrElse(getContent(),
                    FieldName.Schedule.FIELD_CONTENT_DELETE_MORE_LOB_IN_TARGET, false);
            SdbCLTransferHelper sdbCLTransferHelper = new SdbCLTransferHelper(getTaskId(),
                    maxExecTime, taskStartTime, getSourceSite(), getTargetSite(), sourceCL,
                    targetCL, deleteMoreLobInTarget, () -> running);

            BasicBSONList currentSnapshots = SdbHelper.getCLSnapshot(sourceSdb, clFullName);
            if (currentSnapshots == null) {
                throw new ScheduleSystemException(
                        "failed to get collection snapshot, snapshot is null, cl=" + clFullName);
            }

            // 如果记录快照发生了变化，则需要重新迁移数据
            if (needReTransferRecord(clFullName, currentSnapshots)) {
                truncateCLDataAndCleanTransferStatus(clFullName);
                ScheduleServer.getInstance().recordCollectionSnapshot(getSourceSite(), clFullName,
                        currentSnapshots, false, false);
            }

            boolean hasRecord = sourceCL.queryOne() != null;
            if (hasRecord) {
                if (canTransferRecord(clFullName, currentSnapshots)) {
                    // transfer record
                    if (!skipTransfer(clFullName, currentSnapshots, recordSkipStrategy)) {
                        try {
                            // 迁移记录前，就先删掉自增字段
                            dropTargetClAutoInc(targetSiteSdb, targetCL);
                            boolean transferFinish = sdbCLTransferHelper.transferRecord();
                            if (transferFinish) {
                                // 迁移完成，标记记录的快照是有效的
                                ScheduleServer.getInstance()
                                        .updateRecordSnapshotEffective(getSourceSite(), clFullName, true);
                            }
                        } finally {
                            // 迁移记录结束，补上自增字段
                            addOrUpdateAutoInc(sourceSdb, targetSiteSdb, clFullInfo);
                        }
                    } else {
                        logger.debug(
                                "the cl snapshot has not changed and effective, skip transfer record, sourceSite={}, clName={}",
                                getSourceSite(), clFullName);
                    }
                } else {
                    // 记录不符合迁移条件，标记中断
                    interruptionFlag.markInterrupted();
                }
            }
            else {
                // 如果是没有记录的表，那么该表的记录快照始终标记为有效状态
                updateOrSaveRecordEffective(currentSnapshots, clFullName);
            }

            if (checkTimeout() || !running) {
                return;
            }

            // transfer lob
            if (!skipTransfer(clFullName, currentSnapshots, lobSkipStrategy)) {
                boolean transferFinish = sdbCLTransferHelper.transferLob();
                if (transferFinish) {
                    // 迁移完成，标记lob的快照是有效的
                    ScheduleServer.getInstance().updateLobSnapshotEffective(getSourceSite(), clFullName, true);
                }
            }
            else {
                logger.debug("the cl snapshot has not changed and effective, skip transfer lob, sourceSite={}, clName={}", getSourceSite(), clFullName);
            }
        }
        finally {
            if (targetSiteSdb != null) {
                targetSiteDataService.releaseConnection(targetSiteSdb);
            }
        }
    }

    private void updateOrSaveRecordEffective(BasicBSONList currentSnapshots, String clFullName)
            throws Exception {
        boolean updated = ScheduleServer.getInstance()
                .updateRecordSnapshotEffective(getSourceSite(), clFullName, true);
        // 没有更新，说明快照表没有该集合的快照记录，写入一条快照记录，同时标记 record 快照为有效
        if (!updated) {
            ScheduleServer.getInstance().recordCollectionSnapshot(getSourceSite(), clFullName,
                    currentSnapshots, true, false);
        }
    }

    @Override
    public SaveCLSnapshotPreHandleRes saveClSnapshotPreHandle(String clFullName, boolean recordSnapshotSame)
            throws Exception {
        if (recordSnapshotSame) {
            return new SaveCLSnapshotPreHandleRes(false);
        }
        // 更新集合快照信息前，如果发现集合的记录快照发生变化，说明该集合的记录数据发生了变化，此时必须先将数据和记录的迁移状态清空
        truncateCLDataAndCleanTransferStatus(clFullName);
        return new SaveCLSnapshotPreHandleRes(true);
    }

    private void truncateCLDataAndCleanTransferStatus(String clFullName) throws Exception {
        logger.info(
                "the collection record snapshot is changed, truncate cl data, sourceSite={}, targetSite={}, clName={}",
                getSourceSite(), getTargetSite(), clFullName);
        truncateCLFromTarget(clFullName);
        ScheduleServer.getInstance().cleanTransferRecordStatus(getSourceSite(), getTargetSite(),
                clFullName);
    }

    private boolean canTransferRecord(String clFullName, BasicBSONList currentSnapshots) throws Exception {
        int notWriteTimeThreshold = BsonUtils.getIntegerOrElse(getContent(),
                FieldName.Schedule.FIELD_CONTENT_NO_WRITE_TIME_THRESHOLD, 30);

        CollectionSnapshotRecord lastCollectionSnapshotRecord = ScheduleServer.getInstance()
                .getLastCollectionSnapshotRecord(getSourceSite(), clFullName);
        if (lastCollectionSnapshotRecord == null) {
            saveCLSnapshot(getSourceSite(), clFullName, null, currentSnapshots);
            // 当 <=0 时，说明不受限制，随时可以迁移
            return notWriteTimeThreshold <= 0;
        }

        boolean res = hasNewDataWrite(lastCollectionSnapshotRecord.getSnapshots(),
                currentSnapshots);
        if (res) {
            saveCLSnapshot(getSourceSite(), clFullName, lastCollectionSnapshotRecord,
                    currentSnapshots);
            return false;
        }

        // 检查是否超过指定时间没有新数据写入
        long lastRecordTime = lastCollectionSnapshotRecord.getLastRecordTime();
        long currentTime = System.currentTimeMillis();
        if (notWriteTimeThreshold > 0
                && currentTime - lastRecordTime < notWriteTimeThreshold * 24 * 60 * 60 * 1000L) {
            return false;
        }

        return true;
    }

    private void truncateCLFromTarget(String clFullName) throws Exception {
        DataServiceWrapper targetSiteDataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(getTargetSite());
        Sequoiadb targetSiteSdb = null;
        try {
            targetSiteSdb = targetSiteDataService.getConnection();
            SdbHelper.truncateCL(targetSiteSdb, clFullName);
        }
        finally {
            if (targetSiteSdb != null) {
                targetSiteDataService.releaseConnection(targetSiteSdb);
            }
        }
    }

    private boolean needReTransferRecord(String clFullName, BasicBSONList currentSnapshots) throws Exception {
        CollectionSnapshotRecord lastCollectionSnapshotRecord = ScheduleServer.getInstance()
                .getLastCollectionSnapshotRecord(getSourceSite(), clFullName);
        if (lastCollectionSnapshotRecord == null) {
            return false;
        }

        return !compareRecordSnapshot(lastCollectionSnapshotRecord.getSnapshots(), currentSnapshots);
    }

    private boolean skipTransfer(String clFullName, BasicBSONList currentSnapshots, SnapshotSkipStrategy skipStrategy) throws Exception {
        CollectionSnapshotRecord lastCollectionSnapshotRecord = ScheduleServer.getInstance()
                .getLastCollectionSnapshotRecord(getSourceSite(), clFullName);
        if (lastCollectionSnapshotRecord == null) {
            saveCLSnapshot(getSourceSite(), clFullName, null, currentSnapshots);
            return false;
        }
        // 比对快照是否一致
        //   一致：检查快照有效性，有效则跳过迁移，无效，则继续迁移
        //  不一致：保存快照信息，标记快照无效，继续迁移
        if (skipStrategy.compareSnapshot(lastCollectionSnapshotRecord.getSnapshots(), currentSnapshots)) {
            return skipStrategy.isSnapshotEffective(lastCollectionSnapshotRecord);
        }
        else {
            // 最新快照有变化，不能跳过迁移，保存本次采集的快照
            saveCLSnapshot(getSourceSite(), clFullName, lastCollectionSnapshotRecord, currentSnapshots);
            return false;
        }
    }

    private void addOrUpdateAutoInc(Sequoiadb sourceSdb, Sequoiadb targetSiteSdb,
            SdbCLFullInfo clFullInfo) throws ScheduleSystemException {
        String csCLName = clFullInfo.getFullName();
        BasicBSONList sourceAutoIncrements = getAutoIncrements(sourceSdb, csCLName,
                getSourceSite());
        if (sourceAutoIncrements == null || sourceAutoIncrements.isEmpty()) {
            return;
        }

        Set<String> targetAutoIncFields = getAutoIncFields(targetSiteSdb, csCLName,
                getTargetSite());
        DBCollection targetCL = targetSiteSdb.getCollectionSpace(clFullInfo.getCsName())
                .getCollection(clFullInfo.getClName());
        for (Object o : sourceAutoIncrements) {
            BSONObject autoIncrement = (BSONObject) o;
            String field = (String) autoIncrement.get("Field");
            if (targetAutoIncFields.contains(field)) {
                targetCL.setAttributes(new BasicBSONObject("AutoIncrement", autoIncrement));
            }
            else {
                try {
                    targetCL.createAutoIncrement(autoIncrement);
                }
                catch (BaseException e) {
                    if (e.getErrorCode() != SDBError.SDB_AUTOINCREMENT_FIELD_CONFLICT
                            .getErrorCode()) {
                        throw e;
                    }
                }
            }
        }
    }

    private void dropTargetClAutoInc(Sequoiadb targetSiteSdb, DBCollection targetCL)
            throws ScheduleSystemException {
        Set<String> autoIncFields = getAutoIncFields(targetSiteSdb, targetCL.getFullName(),
                getTargetSite());
        for (String autoIncField : autoIncFields) {
            try {
                targetCL.dropAutoIncrement(autoIncField);
            }
            catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_AUTOINCREMENT_FIELD_NOT_EXIST.getErrorCode()) {
                    throw e;
                }
            }
        }
    }

    private void transferMainCl(Sequoiadb sourceSdb, String mainCLName) throws Exception {
        BSONObject mainClCataInfo = SdbHelper.getCataSnapshotByClName(sourceSdb, mainCLName);
        if (mainClCataInfo == null) {
            throw new IllegalArgumentException("main cl not exist in source site, source site="
                    + getSourceSite() + ", mainClName=" + mainCLName);
        }
        _transferMainCl(sourceSdb, mainClCataInfo);
    }

    @Override
    public String getName() {
        return "SDB_TASK_TRANSFER";
    }

    @Override
    public int getTaskType() {
        return ScheduleDefine.TaskType.TRANSFER;
    }
}
interface SnapshotSkipStrategy {

    /**
     * 快照是否有效
     */
    boolean isSnapshotEffective(CollectionSnapshotRecord record);

    /**
     * 快照是否相同
     */
    boolean compareSnapshot(BasicBSONList oldSnapshots, BasicBSONList newSnapshots);
}