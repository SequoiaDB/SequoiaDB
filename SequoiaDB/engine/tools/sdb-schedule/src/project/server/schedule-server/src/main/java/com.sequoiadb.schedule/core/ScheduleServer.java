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

   Source File Name = ScheduleServer.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core;

import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.HashSlotLock;
import com.sequoiadb.schedule.common.LockWrapper;
import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.task.SdbDataSwitchTask;
import com.sequoiadb.schedule.dao.CollectionDataSwitchEventDao;
import com.sequoiadb.schedule.dao.CollectionSnapshotRecordDao;
import com.sequoiadb.schedule.dao.CollectionTransferRecordStatusDao;
import com.sequoiadb.schedule.dao.TaskDao;
import com.sequoiadb.schedule.dao.TaskProgressDao;
import com.sequoiadb.schedule.dao.TransactionFactory;
import com.sequoiadb.schedule.dao.TransactionLockDao;
import com.sequoiadb.schedule.lock.TransactionLock;
import com.sequoiadb.schedule.lock.TransactionLockFactory;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.template.ITransaction;
import com.sequoiadb.schedule.model.CollectionDataSwitchEvent;
import com.sequoiadb.schedule.model.CollectionSnapshotRecord;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import com.sequoiadb.schedule.model.SiteInfo;
import com.sequoiadb.schedule.model.TaskEntity;
import com.sequoiadb.schedule.remote.FeignClientFactory;
import com.sequoiadb.schedule.remote.ScheduleServerFeignClient;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ScheduleServer {
    private static Logger logger = LoggerFactory.getLogger(ScheduleServer.class);
    private static ScheduleServer instance = new ScheduleServer();

    private TaskDao taskDao;
    private FeignClientFactory feignClientFactory;
    private HashSlotLock hashSlotLock;
    private ScheduleNodeMgr scheduleNodeMgr;
    private SiteMgr siteMgr;
    private SiteDataserviceMgr siteDataserviceMgr;
    private TransactionFactory transactionFactory;
    private TransactionLockDao transactionLockDao;
    private CollectionSnapshotRecordDao collectionSnapshotRecordDao;
    private CollectionDataSwitchEventDao collectionDataSwitchEventDao;

    private TransactionLockFactory transactionLockFactory;

    private CollectionTransferRecordStatusDao collectionTransferRecordStatusDao;

    private TaskProgressDao taskProgressDao;

    private ScheduleServer() {

    }

    public void init(TaskDao taskDao, FeignClientFactory feignClientFactory,
            ScheduleNodeMgr scheduleNodeMgr, SiteMgr siteMgr, SiteDataserviceMgr siteDataserviceMgr,
            TransactionFactory transactionFactory, TransactionLockDao transactionLockDao,
            CollectionSnapshotRecordDao collectionSnapshotRecordDao,
            CollectionDataSwitchEventDao collectionDataSwitchEventDao,
            TransactionLockFactory transactionLockFactory,
            CollectionTransferRecordStatusDao collectionTransferRecordStatusDao,
            TaskProgressDao taskProgressDao) {
        this.taskDao = taskDao;
        this.feignClientFactory = feignClientFactory;
        this.scheduleNodeMgr = scheduleNodeMgr;
        this.siteMgr = siteMgr;
        this.siteDataserviceMgr = siteDataserviceMgr;
        this.transactionFactory = transactionFactory;
        this.transactionLockDao = transactionLockDao;
        this.collectionSnapshotRecordDao = collectionSnapshotRecordDao;
        this.collectionDataSwitchEventDao = collectionDataSwitchEventDao;
        this.transactionLockFactory = transactionLockFactory;
        this.collectionTransferRecordStatusDao = collectionTransferRecordStatusDao;
        this.taskProgressDao = taskProgressDao;
        this.hashSlotLock = new HashSlotLock(1000);
    }

    public static ScheduleServer getInstance() {
        return instance;
    }

    public TaskEntity queryLatestTask(BSONObject condition) throws Exception {
        BasicBSONObject orderBy = new BasicBSONObject();
        orderBy.put(FieldName.Task.FIELD_START_TIME, -1);
        return taskDao.queryOne(condition, orderBy);
    }

    public ServerNodeEntity getServer(String runServer) {
        ServerNodeEntity serverNode = scheduleNodeMgr.getServerNode(runServer);
        return serverNode;
    }

    public void insertLockInfo(BSONObject lockInfo) {
        transactionLockDao.upsert(lockInfo);
    }

    public void insertTask(TaskEntity info) throws Exception {
        BSONObject lockInfo = new BasicBSONObject();
        lockInfo.put(FieldName.TransactionLock.FIELD_LOCK_KEY,
                ScheduleDefine.TransactionLockKey.LOCK_TASK);
        lockInfo.put(FieldName.TransactionLock.FIELD_LOCK_VALUE, info.getId());
        ITransaction transaction = transactionFactory.createTransaction();
        try {
            transaction.begin();
            taskDao.insert(info, transaction);
            transactionLockDao.insert(lockInfo, transaction);
            transaction.commit();
        }
        catch (Exception e) {
            transaction.rollback();
            throw e;
        }
    }

    public TransactionLock createTransactionLock(String lockKey, String lockValue) {
        return transactionLockFactory.create(lockKey, lockValue);
    }

    public void updateTaskRunServer(String runServer, String taskId) throws Exception {
        BSONObject newValue = new BasicBSONObject(FieldName.Task.FIELD_RUN_SERVER, runServer);
        taskDao.updateByTaskId(taskId, newValue);
    }

    public void notifyTask(String target, String taskId, int notifyType) {
        getTargetClient(target).notifyTask(taskId, notifyType);
    }

    private ScheduleServerFeignClient getTargetClient(String target) {
        ScheduleServerFeignClient client = feignClientFactory
                .getClient(ScheduleServerFeignClient.class, target);
        return client;
    }

    public ServerNodeEntity getActiveServer() {
        return scheduleNodeMgr.getRandomAliveNode();
    }

    public ServerNodeEntity getLocalServer() {
        return scheduleNodeMgr.getLocalServer();
    }

    public boolean isLocalServer(ServerNodeEntity server) {
        ServerNodeEntity localServer = scheduleNodeMgr.getLocalServer();
        return server.getHostName().equals(localServer.getHostName())
                && server.getIpAddress().equals(localServer.getIpAddress())
                && server.getPort() == localServer.getPort();
    }

    public LockWrapper getWriteLock(String scheduleId) {
        return hashSlotLock.getWriteLock(scheduleId);
    }

    public SiteInfo getSite(String name) throws Exception {
        return siteMgr.getSite(name);
    }

    public CollectionSnapshotRecord getLastCollectionSnapshotRecord(String siteName,
            String collectionName) throws Exception {
        return collectionSnapshotRecordDao.findOne(siteName, collectionName);
    }

    public boolean updateRecordSnapshotEffective(String siteName, String clFullName, boolean recordSnapshotEffective) {
        UpdateResult updateResult = collectionSnapshotRecordDao.updateRecordSnapshotEffective(siteName, clFullName,
                recordSnapshotEffective);
        return updateResult.getUpdatedNum() != 0;
    }

    public void updateLobSnapshotEffective(String siteName, String clFullName, boolean lobSnapshotEffective) {
        collectionSnapshotRecordDao.updateLobSnapshotEffective(siteName, clFullName, lobSnapshotEffective);
    }

    public void recordCollectionSnapshot(String siteName, String collectionName,
            BasicBSONList snapshots, boolean recordSnapshotEffective, boolean lobSnapshotEffective)
            throws Exception {
        collectionSnapshotRecordDao.upsert(siteName, collectionName, snapshots,
                recordSnapshotEffective, lobSnapshotEffective);
    }

    public void insertDataSwitchEvent(CollectionDataSwitchEvent event) throws Exception {
        collectionDataSwitchEventDao.insert(event);
    }

    public void updateTask(BSONObject matcher, BSONObject modifier) throws Exception {
        taskDao.update(matcher, modifier);
    }

    public DataServiceWrapper getDataServiceWrapper(String siteName) throws Exception {
        return siteDataserviceMgr.getDataService(siteName);
    }

    public void upsertClTransferRecordStatus(String sourceSite, String targetSite, String clName,
            String startId, String endId) {
        collectionTransferRecordStatusDao.upsert(sourceSite, targetSite, clName, startId, endId);
    }

    public BSONObject getClTransferRecordStatus(String sourceSite, String targetSite,
            String clName) {
        return collectionTransferRecordStatusDao.queryOne(sourceSite, targetSite, clName);
    }

    public void updateTaskTransferProgress(String taskId, String clName, long successRecordNum,
            long failedRecordNum, long transferRecordSize, long successLobNum, long failedLobNum,
            long transferLobSize) throws Exception {
        taskProgressDao.updateTaskProgress(taskId, clName, successRecordNum, failedRecordNum,
                transferRecordSize, successLobNum, failedLobNum, transferLobSize);
    }

    public void saveTaskDataSwitchProgress(String taskId, String clName,
            SdbDataSwitchTask.DataSwitchRes dataSwitchRes) {
        BSONObject taskProgress = new BasicBSONObject();
        taskProgress.put(FieldName.TaskProgress.FIELD_TASK_ID, taskId);
        taskProgress.put(FieldName.TaskProgress.FIELD_COLLECTION_NAME, clName);
        taskProgress.put(FieldName.TaskProgress.FIELD_CAN_DATA_SWITCH,
                dataSwitchRes.isCanDataSwitch());
        taskProgress.put(FieldName.TaskProgress.FIELD_DATA_SWITCHED,
                dataSwitchRes.isDataSwitched());
        taskProgressDao.saveTaskProgress(taskProgress);
    }

    public void updateCollectionDataSwitchEvent(CollectionDataSwitchEvent event,
            BSONObject newValue) throws Exception {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_COLLECTION_NAME,
                event.getCollection());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_SITE_NAME,
                event.getSourceSite());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_SITE_NAME,
                event.getTargetSite());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_TARGET_DATASOURCE_NAME,
                event.getTargetDatasource());
        matcher.put(FieldName.CollectionDataSwitchEvent.FIELD_SOURCE_CL_RENAME, event.getRename());
        BSONObject modifier = new BasicBSONObject("$set", newValue);
        collectionDataSwitchEventDao.update(matcher, modifier);
    }

    public MetaCursor queryCollectionDataSwitchEvent(BSONObject condition) throws Exception {
        return collectionDataSwitchEventDao.query(condition);
    }

    public void saveTaskCleanProgress(String taskId, String site, String sourceCL, String renameCl,
            Long cleanTime, boolean success, String detail) {
        BSONObject taskProgress = new BasicBSONObject();
        taskProgress.put(FieldName.TaskProgress.FIELD_TASK_ID, taskId);
        taskProgress.put(FieldName.TaskProgress.FIELD_CLEAN_SITE, site);
        taskProgress.put(FieldName.TaskProgress.FIELD_SOURCE_CL, sourceCL);
        taskProgress.put(FieldName.TaskProgress.FIELD_RENAME_CL, renameCl);
        taskProgress.put(FieldName.TaskProgress.FIELD_CLEAN_TIME, cleanTime);
        taskProgress.put(FieldName.TaskProgress.FIELD_CLEAN_SUCCESS, success);
        taskProgress.put(FieldName.TaskProgress.FIELD_CLEAN_DETAIL, detail);
        taskProgressDao.saveTaskProgress(taskProgress);
    }

    public TaskEntity getTask(String taskId) throws Exception {
        return taskDao.queryById(taskId);
    }

    public void cleanTransferRecordStatus(String sourceSite, String targetSite, String clFullName) {
        collectionTransferRecordStatusDao.delete(sourceSite, targetSite, clFullName, null);
    }

    public void cleanTransferStatus(String sourceSite, String targetSite, String clFullName)
            throws Exception {
        ITransaction transaction = transactionFactory.createTransaction();
        try {
            transaction.begin();
            collectionTransferRecordStatusDao.delete(sourceSite, targetSite, clFullName,
                    transaction);
            collectionSnapshotRecordDao.delete(sourceSite, clFullName, transaction);
            transaction.commit();
        }
        catch (Exception e) {
            transaction.rollback();
            throw e;
        }
    }
}
