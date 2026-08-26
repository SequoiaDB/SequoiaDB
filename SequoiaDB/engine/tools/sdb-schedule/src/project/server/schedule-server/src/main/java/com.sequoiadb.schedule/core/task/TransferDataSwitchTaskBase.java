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

   Source File Name = TransferDataSwitchTaskBase.java

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
import com.sequoiadb.base.DBVersion;
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
import com.sequoiadb.schedule.model.SdbScheduleTaskPlanInfo;
import com.sequoiadb.schedule.model.SiteInfo;
import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.util.CollectionUtils;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public abstract class TransferDataSwitchTaskBase extends ScheduleTaskBase {
    private static Logger logger = LoggerFactory.getLogger(TransferDataSwitchTaskBase.class);
    protected long taskStartTime = 0;
    protected volatile boolean running = true;
    private String taskId;
    private TaskEntity taskEntity;
    private String sourceSite;
    private String targetSite;
    private List<String> csList;
    private List<String> clList;
    private List<String> csRegexList;
    private List<String> clRegexList;
    private TransactionLock transactionLock;
    private SdbScheduleTaskPlanInfo plan;
    private String datasourceName;
    protected long maxExecTime;
    protected long processClCount = 0;

    protected boolean isTimeout = false;
    private static final String[] recordSnapshotFields = { "NodeName", "GroupName", "TotalRecords",
            "TotalDataFreeSpace", "TotalDataPages", "DataCommitLSN" };
    private static final String[] lobSnapshotFields = { "NodeName", "GroupName", "TotalLobs",
            "TotalLobPages", "LobCommitLSN", "TotalValidLobSize" };

    public TransferDataSwitchTaskBase(ScheduleTaskMgr mgr, TaskEntity taskEntity,
            TransactionLock transactionLock) {
        super(mgr);
        this.transactionLock = transactionLock;
        this.taskEntity = taskEntity;
        this.taskId = taskEntity.getId();
        BSONObject content = taskEntity.getContent();
        sourceSite = BsonUtils.getStringChecked(content,
                FieldName.Schedule.FIELD_CONTENT_SOURCE_SITE);
        targetSite = BsonUtils.getStringChecked(content,
                FieldName.Schedule.FIELD_CONTENT_TARGET_SITE);
        csList = BsonUtils.getStringArray(content, FieldName.Schedule.FIELD_CONTENT_CS_LIST);
        clList = BsonUtils.getStringArray(content, FieldName.Schedule.FIELD_CONTENT_CL_LIST);
        csRegexList = BsonUtils.getStringArray(content, FieldName.Schedule.FIELD_CONTENT_CS_REGEX);
        clRegexList = BsonUtils.getStringArray(content, FieldName.Schedule.FIELD_CONTENT_CL_REGEX);
        maxExecTime = BsonUtils.getLongOrElse(content, FieldName.Schedule.FIELD_MAX_EXEC_TIME, 0);
    }

    @Override
    public final void _runTask() {
        taskStartTime = System.currentTimeMillis();
        try {
            logger.info("running task, task={}", getTaskInfo().toString());
            updateTaskRunning(taskId, taskStartTime);
            checkSite();
            checkSdbVersion();
            plan = generatePlan();
            updateTaskPlan(plan);
            doTask();
        }
        catch (Exception e) {
            logger.warn("run task failed:taskId={}", getTaskId(), e);
            abortTaskAndAsyncRedo(getTaskId(), ScheduleDefine.TaskRunningFlag.SCHEDULE_TASK_ABORT,
                    e.toString(), processClCount);
        }
        finally {
            transactionLock.unlock();
        }
    }

    public abstract void checkSdbVersion() throws Exception;

    protected Version getDataServiceVersion(String siteName) throws Exception {
        DataServiceWrapper dataService = ScheduleServer.getInstance()
                .getDataServiceWrapper(siteName);
        Sequoiadb sdb = null;
        try {
            sdb = dataService.getConnection();
            DBVersion dbVersion = sdb.getDBVersion();
            return new Version(dbVersion.getVersion(), dbVersion.getSubVersion(),
                    dbVersion.getFixVersion());
        }
        finally {
            if (sdb != null) {
                dataService.releaseConnection(sdb);
            }
        }
    }

    private void checkSite() throws ScheduleSystemException {
        try {
            SiteInfo site = ScheduleServer.getInstance().getSite(sourceSite);
            if (site == null) {
                throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                        "source site not exist:" + sourceSite);
            }
            site = ScheduleServer.getInstance().getSite(targetSite);
            if (site == null) {
                throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                        "target site not exist:" + targetSite);
            }
            String datasource = site.getDatasource();
            if (datasource == null || datasource.isEmpty()) {
                throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                        "target site is not a datasource:" + targetSite);
            }

            DataServiceWrapper dataService = ScheduleServer.getInstance()
                    .getDataServiceWrapper(sourceSite);
            Sequoiadb sdb = null;
            try {
                sdb = dataService.getConnection();
                boolean dataSourceExist = sdb.isDataSourceExist(datasource);
                if (!dataSourceExist) {
                    throw new ScheduleServerException(ScheduleServerError.RECORD_NOT_EXISTS,
                            "datasource not exist in source site, source site=" + sourceSite
                                    + ", datasource=" + datasourceName);
                }
            }
            finally {
                if (sdb != null) {
                    dataService.releaseConnection(sdb);
                }
            }
            this.datasourceName = site.getDatasource();
        }
        catch (Exception e) {
            throw new ScheduleSystemException("check site info failed, " + e.getMessage(), e);
        }
    }

    protected boolean canSchedule(BSONObject clCataInfo) throws Exception {
        String createTime = BsonUtils.getString(clCataInfo, "CreateTime");
        if (createTime == null) {
            return true;
        }
        BSONObject content = getContent();
        int clCreateTimeThreshold = BsonUtils.getIntegerOrElse(content,
                FieldName.Schedule.FIELD_CONTENT_CL_CREATE_TIME_THRESHOLD, 30);
        if (clCreateTimeThreshold <= 0) {
            return true;
        }
        long time = parseTime(createTime.substring(0, createTime.length() - 3)).getTime();
        long thresholdTime = System.currentTimeMillis()
                - clCreateTimeThreshold * 24 * 60 * 60 * 1000L;
        return time <= thresholdTime;
    }

    private Date parseTime(String time) throws ParseException {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd-HH.mm.ss.SSS");
        return sdf.parse(time);
    }

    protected DBCursor getClCataSnapshot(Sequoiadb db) {
        BasicBSONList orList = new BasicBSONList();
        Set<String> csSet = plan.getCsSet();
        if (!CollectionUtils.isEmpty(csSet)) {
            Set<String> regexCsSet = new HashSet<>();
            for (String cs : csSet) {
                regexCsSet.add("^" + cs + ".");
            }
            String csRegex = String.join("|", regexCsSet);
            BSONObject regexMatcher = new BasicBSONObject();
            regexMatcher.put("$regex", csRegex);
            orList.add(new BasicBSONObject("Name", regexMatcher));
        }
        Set<String> clSet = plan.getClSet();
        if (!CollectionUtils.isEmpty(clSet)) {
            BSONObject inMatcher = new BasicBSONObject();
            inMatcher.put("$in", clSet);
            orList.add(new BasicBSONObject("Name", inMatcher));
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put("$or", orList);

        BasicBSONList notList = new BasicBSONList();
        notList.add(new BasicBSONObject("Name",
                new BasicBSONObject("$regex", "_data_switch_bak_\\d+$")));
        matcher.put("$not", notList);

        matcher.put("DataSourceID", new BasicBSONObject("$exists", 0));

        return db.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG, matcher, null,
                new BasicBSONObject("CreateTime", 1));
    }

    protected CLPartitionSortedRes getClAndSortByPartition(Sequoiadb sourceSdb) {
        List<String> commonAndMainClList = new ArrayList<>();
        // key: mainClName, value: subClName list
        Map<String, Set<String>> subClMap = new HashMap<>();
        try (DBCursor cursor = getClCataSnapshot(sourceSdb)) {
            while (cursor.hasNext()) {
                BSONObject clCataInfo = cursor.getNext();
                String clFullName = BsonUtils.getStringChecked(clCataInfo, "Name");
                if (clCataInfo.containsField("IsMainCL")) {
                    commonAndMainClList.add(clFullName);
                    continue;
                }

                if (clCataInfo.containsField("MainCLName")) {
                    String mainClName = BsonUtils.getStringChecked(clCataInfo, "MainCLName");
                    subClMap.computeIfAbsent(mainClName, k -> new HashSet<>()).add(clFullName);
                }
                else {
                    commonAndMainClList.add(clFullName);
                }
            }
        }
        // key: mainClName, value: partition sorted subClName list
        Map<String, List<String>> sortedSubClMap = new HashMap<>();
        for (Map.Entry<String, Set<String>> entry : subClMap.entrySet()) {
            if (entry.getValue().size() <= 1) {
                sortedSubClMap.put(entry.getKey(), new ArrayList<>(entry.getValue()));
                continue;
            }

            String mainClName = entry.getKey();
            Set<String> subClSet = entry.getValue();
            try (DBCursor cursor = sourceSdb.exec(
                    "select t1.CataInfo from (select CataInfo from $SNAPSHOT_CATA where Name='"
                            + mainClName
                            + "' split by CataInfo) as t1 order by t1.CataInfo.UpBound")) {
                while (cursor.hasNext()) {
                    BSONObject obj = cursor.getNext();
                    BSONObject cataInfo = BsonUtils.getBSON(obj, "CataInfo");
                    String subClName = BsonUtils.getStringChecked(cataInfo, "SubCLName");
                    if (subClSet.contains(subClName)) {
                        sortedSubClMap.computeIfAbsent(mainClName, k -> new ArrayList<>())
                                .add(subClName);
                    }
                }
            }
        }
        return new CLPartitionSortedRes(sortedSubClMap, commonAndMainClList);
    }

    public static class CLPartitionSortedRes {
        private Map<String, List<String>> sortedSubClMap;
        private List<String> commonAndMainClList;

        public CLPartitionSortedRes(Map<String, List<String>> sortedSubClMap,
                List<String> commonAndMainClList) {
            this.sortedSubClMap = sortedSubClMap;
            this.commonAndMainClList = commonAndMainClList;
        }

        public List<String> getCommonAndMainClList() {
            return commonAndMainClList;
        }

        public Map<String, List<String>> getSortedSubClMap() {
            return sortedSubClMap;
        }
    }

    private void updateTaskPlan(SdbScheduleTaskPlanInfo plan) throws Exception {
        try {
            TaskUpdator updator = new TaskPlanUpdator(taskId, plan.toBSONObject());
            updator.doUpdate();
        }
        catch (Exception e) {
            logger.error("update task plan failed:taskId=" + getTaskId(), e);
            throw e;
        }
    }

    private SdbScheduleTaskPlanInfo generatePlan() throws ScheduleServerException {
        try {
            DataServiceWrapper dataService = ScheduleServer.getInstance()
                    .getDataServiceWrapper(sourceSite);
            Sequoiadb sdb = null;
            try {
                sdb = dataService.getConnection();
                Set<String> csSet = new HashSet<>(csList);
                if (!CollectionUtils.isEmpty(csRegexList)) {
                    csSet.addAll(SdbHelper.getCSList(sdb, csRegexList));
                }
                Set<String> clSet = new HashSet<>(clList);
                if (!CollectionUtils.isEmpty(clRegexList)) {
                    clSet.addAll(SdbHelper.getCLList(sdb, clRegexList));
                }
                SdbScheduleTaskPlanInfo planInfo = new SdbScheduleTaskPlanInfo();
                planInfo.setCsSet(csSet);
                planInfo.setClSet(clSet);
                return planInfo;
            }
            finally {
                if (sdb != null) {
                    dataService.releaseConnection(sdb);
                }
            }
        }
        catch (Exception e) {
            throw new ScheduleSystemException("generate plan failed", e);
        }
    }

    public boolean checkTimeout() {
        if (isTimeout) {
            return true;
        }
        if (maxExecTime > 0 && System.currentTimeMillis() - taskStartTime > maxExecTime) {
            this.isTimeout = true;
        }
        return isTimeout;
    }

    public TransactionLock createCollectionTransactionLock(String clName)
            throws ScheduleServerException {
        try {
            BSONObject lockInfo = new BasicBSONObject();
            lockInfo.put(FieldName.TransactionLock.FIELD_LOCK_KEY,
                    ScheduleDefine.TransactionLockKey.LOCK_COLLECTION);
            lockInfo.put(FieldName.TransactionLock.FIELD_LOCK_VALUE, clName);
            ScheduleServer.getInstance().insertLockInfo(lockInfo);
            return ScheduleServer.getInstance().createTransactionLock(
                    ScheduleDefine.TransactionLockKey.LOCK_COLLECTION, clName);
        }
        catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_TIMEOUT.getErrorCode()) {
                throw new ScheduleServerException(ScheduleServerError.TRANSACTION_LOCK_TIMEOUT,
                        "failed to create collection transaction lock, cl=" + clName, e);
            }
            throw new ScheduleSystemException(
                    "failed to create collection transaction lock, cl=" + clName, e);
        }
    }

    public boolean notDataWrite(Sequoiadb sourceSdb, String clFullName) throws Exception {
        BasicBSONList currentSnapshots = SdbHelper.getCLSnapshot(sourceSdb, clFullName);
        if (currentSnapshots == null) {
            throw new ScheduleSystemException(
                    "failed to get collection snapshot, snapshot is null, cl=" + clFullName);
        }

        CollectionSnapshotRecord lastCollectionSnapshotRecord = ScheduleServer.getInstance()
                .getLastCollectionSnapshotRecord(getSourceSite(), clFullName);
        if (lastCollectionSnapshotRecord == null) {
            return false;
        }

        boolean res = hasNewDataWrite(lastCollectionSnapshotRecord.getSnapshots(),
                currentSnapshots);
        if (res) {
            return false;
        }

        // 没有新数据写入
        long lastRecordTime = lastCollectionSnapshotRecord.getLastRecordTime();
        int notWriteTimeThreshold = BsonUtils.getIntegerOrElse(getContent(),
                FieldName.Schedule.FIELD_CONTENT_NO_WRITE_TIME_THRESHOLD, 30);
        long currentTime = System.currentTimeMillis();
        if (notWriteTimeThreshold > 0
                && currentTime - lastRecordTime < notWriteTimeThreshold * 24 * 60 * 60 * 1000L) {
            // 没有达到可数据切换的时间阈值
            return false;
        }

        BSONObject cataInfo = SdbHelper.getCataSnapshotByClName(sourceSdb, clFullName);
        Boolean repairCheck = BsonUtils.getBooleanOrElse(cataInfo, "RepairCheck", false);
        if (repairCheck) {
            BasicBSONList newSnapshots = SdbHelper.getCLSnapshot(sourceSdb, clFullName);
            if (newSnapshots == null) {
                throw new ScheduleSystemException(
                        "failed to get collection snapshot, snapshot is null, cl=" + clFullName);
            }
            res = hasNewDataWrite(lastCollectionSnapshotRecord.getSnapshots(), newSnapshots);
            if (res) {
                logger.info("has new data write in repairCheck, cl={}, lastRecordSnapshot={}, newSnapshots={}", clFullName,
                        lastCollectionSnapshotRecord, newSnapshots);
                return false;
            }
            return true;
        }
        else {
            SdbCLFullInfo sdbCLFullInfo = new SdbCLFullInfo(clFullName);
            DBCollection collection = sourceSdb.getCollectionSpace(sdbCLFullInfo.getCsName())
                    .getCollection(sdbCLFullInfo.getClName());

            logger.info("set RepairCheck for collection, site={}, cl={}", sourceSite, clFullName);
            SdbHelper.setClRepairCheck(collection);
            try {
                // 睡眠等待60s，保证正在写的提交完成
                Thread.sleep(60 * 1000);
            }
            catch (Exception e) {
                SdbHelper.unSetClRepairCheck(collection);
                throw e;
            }

            BasicBSONList newSnapshots = SdbHelper.getCLSnapshot(sourceSdb, clFullName);
            if (newSnapshots == null) {
                throw new ScheduleSystemException(
                        "the collection snapshot is null, cl=" + clFullName);
            }
            res = hasNewDataWrite(lastCollectionSnapshotRecord.getSnapshots(), newSnapshots);
            if (res) {
                logger.info("has new data write in repairCheck, cl={}, lastRecordSnapshot={}, newSnapshots={}", clFullName,
                        lastCollectionSnapshotRecord, newSnapshots);
                SdbHelper.unSetClRepairCheck(collection);
                return false;
            }
            return true;
        }
    }

    protected void saveCLSnapshot(String siteName, String collectionName,
            CollectionSnapshotRecord lastCollectionSnapshotRecord, BasicBSONList newSnapshots)
            throws Exception {
        if (lastCollectionSnapshotRecord == null) {
            ScheduleServer.getInstance().recordCollectionSnapshot(siteName, collectionName,
                    newSnapshots, false, false);
            return;
        }

        BasicBSONList lastSnapshots = lastCollectionSnapshotRecord.getSnapshots();
        boolean recordSnapshotSame = compareRecordSnapshot(lastSnapshots, newSnapshots);
        boolean lobSnapshotSame = compareLobSnapshot(lastSnapshots, newSnapshots);
        boolean recordSnapshotEffective = false;
        if (recordSnapshotSame) {
            recordSnapshotEffective = lastCollectionSnapshotRecord.isRecordSnapshotEffective();
        }
        boolean lobSnapshotEffective = false;
        if (lobSnapshotSame) {
            lobSnapshotEffective = lastCollectionSnapshotRecord.isLobSnapshotEffective();
        }
        SaveCLSnapshotPreHandleRes saveCLSnapshotPreHandleRes = saveClSnapshotPreHandle(collectionName, recordSnapshotSame);
        if (saveCLSnapshotPreHandleRes != null && saveCLSnapshotPreHandleRes.isClTruncated()) {
            // 如果目标站点上的集合发生了 truncate，意味着 lob 需要重新迁移，这里直接标记 lob快照的有效性为 false
            lobSnapshotEffective = false;
        }
        ScheduleServer.getInstance().recordCollectionSnapshot(siteName, collectionName,
                newSnapshots, recordSnapshotEffective, lobSnapshotEffective);

    }

    public SaveCLSnapshotPreHandleRes saveClSnapshotPreHandle(String clFullName, boolean recordSnapshotSame) throws Exception {
        return null;
    }

    protected boolean compareLobSnapshot(BasicBSONList leftSnapshots,
            BasicBSONList rightSnapshots) {
        return compareSnapshot(leftSnapshots, rightSnapshots, lobSnapshotFields);
    }

    protected boolean compareRecordSnapshot(BasicBSONList leftSnapshots,
            BasicBSONList rightSnapshots) {
        return compareSnapshot(leftSnapshots, rightSnapshots, recordSnapshotFields);
    }

    private boolean compareSnapshot(BasicBSONList leftSnapshots, BasicBSONList rightSnapshots,
            String[] fields) {
        if (leftSnapshots == null || rightSnapshots == null) {
            return false;
        }

        if (leftSnapshots.size() != rightSnapshots.size()) {
            return false;
        }

        BasicBSONList l = extractFields(leftSnapshots, fields);
        BasicBSONList r = extractFields(rightSnapshots, fields);
        return itemsEquals(l, r);
    }

    private BasicBSONList extractFields(BasicBSONList snapshots, String[] fields) {
        BasicBSONList list = new BasicBSONList();

        for (Object obj : snapshots) {
            BSONObject snapshot = (BSONObject) obj;
            String name = BsonUtils.getStringChecked(snapshot, "Name");
            Number uniqueID = BsonUtils.getNumberChecked(snapshot, "UniqueID");
            String collectionSpace = BsonUtils.getStringChecked(snapshot, "CollectionSpace");
            BasicBSONList details = extractDetailsFields((BasicBSONList) snapshot.get("Details"),
                    fields);

            BSONObject extractedSnapshot = new BasicBSONObject();
            extractedSnapshot.put("Name", name);
            extractedSnapshot.put("UniqueID", uniqueID);
            extractedSnapshot.put("CollectionSpace", collectionSpace);
            extractedSnapshot.put("Details", details);

            list.add(extractedSnapshot);
        }
        return list;
    }

    private BasicBSONList extractDetailsFields(BasicBSONList details, String[] fields) {
        if (details == null) {
            return null;
        }

        BasicBSONList newDetails = new BasicBSONList();
        for (Object obj : details) {
            BSONObject detail = (BSONObject) obj;
            BSONObject newDetail = new BasicBSONObject();
            for (String field : fields) {
                newDetail.put(field, detail.get(field));
            }
            newDetails.add(newDetail);
        }
        return newDetails;
    }

    // 比对集合快照检查是否有新数据写入
    protected boolean hasNewDataWrite(BasicBSONList leftSnapshots, BasicBSONList rightSnapshots) {
        if (clSnapshotEquals(leftSnapshots, rightSnapshots)) {
            return false;
        }
        return true;
    }

    private boolean clSnapshotEquals(BasicBSONList leftSnapshots, BasicBSONList rightSnapshots) {
        if (leftSnapshots == null || rightSnapshots == null) {
            return false;
        }

        if (leftSnapshots.size() != rightSnapshots.size()) {
            return false;
        }

        return itemsEquals(leftSnapshots, rightSnapshots);
    }

    private boolean itemsEquals(BasicBSONList lefts, BasicBSONList rights) {
        if (lefts == null || rights == null) {
            return false;
        }

        if (lefts.size() != rights.size()) {
            return false;
        }

        for (Object l : lefts) {
            BSONObject lItem = (BSONObject) l;
            boolean matchFound = false;
            for (Object r : rights) {
                BSONObject rItem = (BSONObject) r;
                if (lItem.equals(rItem)) {
                    matchFound = true;
                    break;
                }
            }
            if (!matchFound) {
                return false;
            }
        }
        return true;
    }

    public BSONObject generateCreateClOptions(Sequoiadb sourceSdb, BSONObject clCataInfo,
            SdbCLFullInfo sdbCLFullInfo) throws ScheduleServerException {
        BSONObject options = new BasicBSONObject();
        if (clCataInfo.containsField("ShardingKey")) {
            options.put("ShardingKey", clCataInfo.get("ShardingKey"));
        }
        if (clCataInfo.containsField("ShardingType")) {
            options.put("ShardingType", clCataInfo.get("ShardingType"));
        }
        if (clCataInfo.containsField("Partition")) {
            options.put("Partition", clCataInfo.get("Partition"));
        }
        if (clCataInfo.containsField("ReplSize")) {
            options.put("ReplSize", clCataInfo.get("ReplSize"));
        }
        if (clCataInfo.containsField("ConsistencyStrategy")) {
            options.put("ConsistencyStrategy", clCataInfo.get("ConsistencyStrategy"));
        }
        if (clCataInfo.containsField("CompressionTypeDesc")) {
            options.put("CompressionType", clCataInfo.get("CompressionTypeDesc"));
            options.put("Compressed", true);
        }
        else {
            options.put("Compressed", false);
        }
        if (clCataInfo.containsField("AutoSplit")) {
            options.put("AutoSplit", clCataInfo.get("AutoSplit"));
        }
        if (clCataInfo.containsField("AttributeDesc")) {
            String attributeDesc = BsonUtils.getString(clCataInfo, "AttributeDesc");
            if (attributeDesc.contains("StrictDataMode")) {
                options.put("StrictDataMode", true);
            }
            if (attributeDesc.contains("NoIDIndex")) {
                options.put("AutoIndexId", false);
            }
        }
        if (clCataInfo.containsField("LobShardingKeyFormat")) {
            options.put("LobShardingKeyFormat", clCataInfo.get("LobShardingKeyFormat"));
        }
        if (clCataInfo.containsField("IsMainCL")) {
            options.put("IsMainCL", clCataInfo.get("IsMainCL"));
        }
        if (clCataInfo.containsField("EnsureShardingIndex")) {
            options.put("EnsureShardingIndex", clCataInfo.get("EnsureShardingIndex"));
        }
        fillAutoIncrementConf(sourceSdb, options, clCataInfo);
        return options;
    }

    private void fillAutoIncrementConf(Sequoiadb sourceSdb, BSONObject clOptions,
            BSONObject clCataInfo) throws ScheduleServerException {
        BasicBSONList autoIncrements = getAutoIncrements(sourceSdb, clCataInfo, getSourceSite());
        if (autoIncrements != null && !autoIncrements.isEmpty()) {
            clOptions.put("AutoIncrement", autoIncrements);
        }
    }

    public Set<String> getAutoIncFields(Sequoiadb sdb, String csClName, String siteName)
            throws ScheduleSystemException {
        BSONObject clCataInfo = SdbHelper.getCataSnapshotByClName(sdb, csClName);
        if (clCataInfo == null) {
            throw new ScheduleSystemException(
                    "the cl cataInfo not found in site, site=" + siteName + ", cl=" + csClName);
        }
        return getAutoIncFields(clCataInfo);
    }

    public Set<String> getAutoIncFields(BSONObject clCataInfo) {
        Set<String> set = new HashSet<>();
        if (!clCataInfo.containsField("AutoIncrement")) {
            return set;
        }
        BasicBSONList autoIncrementList = (BasicBSONList) clCataInfo.get("AutoIncrement");
        if (autoIncrementList == null || autoIncrementList.isEmpty()) {
            return set;
        }

        for (Object o : autoIncrementList) {
            BSONObject autoIncInfo = (BSONObject) o;
            String field = BsonUtils.getStringChecked(autoIncInfo, "Field");
            set.add(field);
        }
        return set;
    }

    public BasicBSONList getAutoIncrements(Sequoiadb sdb, String clFullName, String siteName)
            throws ScheduleSystemException {
        BSONObject clCataInfo = SdbHelper.getCataSnapshotByClName(sdb, clFullName);
        if (clCataInfo == null) {
            throw new ScheduleSystemException(
                    "the cl cataInfo not found in site, site=" + siteName + ", cl=" + clFullName);
        }
        return getAutoIncrements(sdb, clCataInfo, siteName);
    }

    public BasicBSONList getAutoIncrements(Sequoiadb sdb, BSONObject clCataInfo, String siteName)
            throws ScheduleSystemException {
        BasicBSONList autoIncFields = new BasicBSONList();
        if (!clCataInfo.containsField("AutoIncrement")) {
            return autoIncFields;
        }
        BasicBSONList autoIncrementList = (BasicBSONList) clCataInfo.get("AutoIncrement");
        if (autoIncrementList == null || autoIncrementList.isEmpty()) {
            return autoIncFields;
        }

        for (Object o : autoIncrementList) {
            BSONObject autoIncInfo = (BSONObject) o;
            String sequenceName = BsonUtils.getStringChecked(autoIncInfo, "SequenceName");
            String field = BsonUtils.getStringChecked(autoIncInfo, "Field");
            String generated = BsonUtils.getStringChecked(autoIncInfo, "Generated");
            BSONObject sequencesSnapshot = SdbHelper.getSequencesSnapshot(sdb, sequenceName);
            if (sequencesSnapshot == null) {
                throw new ScheduleSystemException(
                        "failed to get sequence snapshot, snapshot is null, site=" + siteName
                                + ", sequence=" + sequenceName);
            }
            Object increment = sequencesSnapshot.get("Increment");
            Object startValue = sequencesSnapshot.get("StartValue");
            Object minValue = sequencesSnapshot.get("MinValue");
            Object maxValue = sequencesSnapshot.get("MaxValue");
            Object cacheSize = sequencesSnapshot.get("CacheSize");
            Object acquireSize = sequencesSnapshot.get("AcquireSize");
            Object cycled = sequencesSnapshot.get("Cycled");

            BSONObject autoIncField = new BasicBSONObject();
            autoIncField.put("Field", field);
            autoIncField.put("Increment", increment);
            autoIncField.put("StartValue", startValue);
            autoIncField.put("MinValue", minValue);
            autoIncField.put("MaxValue", maxValue);
            autoIncField.put("CacheSize", cacheSize);
            autoIncField.put("AcquireSize", acquireSize);
            autoIncField.put("Cycled", cycled);
            autoIncField.put("Generated", generated);
            autoIncFields.add(autoIncField);
        }
        return autoIncFields;
    }

    protected BSONObject compareAndReturnNewOption(BSONObject sourceCataInfo,
            BSONObject targetCataInfo, String clFullName) throws ScheduleSystemException {
        if (sourceCataInfo == null) {
            throw new ScheduleSystemException("the cl cataInfo not found in site, site="
                    + getSourceSite() + ", cl=" + clFullName);
        }

        if (targetCataInfo == null) {
            throw new ScheduleSystemException("the cl cataInfo not found in site, site="
                    + getTargetSite() + ", cl=" + clFullName);
        }

        BSONObject newOption = new BasicBSONObject();

        if (isDiff(sourceCataInfo.get("ReplSize"), targetCataInfo.get("ReplSize"))) {
            newOption.put("ReplSize", sourceCataInfo.get("ReplSize"));
        }

        if (isDiff(sourceCataInfo.get("ConsistencyStrategy"),
                targetCataInfo.get("ConsistencyStrategy"))) {
            newOption.put("ConsistencyStrategy", sourceCataInfo.get("ConsistencyStrategy"));
        }

        if (isDiff(sourceCataInfo.get("ShardingKey"), targetCataInfo.get("ShardingKey"))) {
            newOption.put("ShardingKey", sourceCataInfo.get("ShardingKey"));
        }

        if (isDiff(sourceCataInfo.get("ShardingType"), targetCataInfo.get("ShardingType"))) {
            newOption.put("ShardingType", sourceCataInfo.get("ShardingType"));
        }

        if (isDiff(sourceCataInfo.get("Partition"), targetCataInfo.get("Partition"))) {
            newOption.put("Partition", sourceCataInfo.get("Partition"));
        }

        if (isDiff(sourceCataInfo.get("AutoSplit"), targetCataInfo.get("AutoSplit"))) {
            newOption.put("AutoSplit", sourceCataInfo.get("AutoSplit"));
        }

        if (isDiff(sourceCataInfo.get("EnsureShardingIndex"),
                targetCataInfo.get("EnsureShardingIndex"))) {
            newOption.put("EnsureShardingIndex", sourceCataInfo.get("EnsureShardingIndex"));
        }

        if (isDiff(sourceCataInfo.get("CompressionTypeDesc"),
                targetCataInfo.get("CompressionTypeDesc"))) {
            if (sourceCataInfo.containsField("CompressionTypeDesc")) {
                newOption.put("CompressionType", sourceCataInfo.get("CompressionTypeDesc"));
                newOption.put("Compressed", true);
            }
            else {
                newOption.put("Compressed", false);
            }
        }

        if (isDiff(sourceCataInfo.get("AttributeDesc"), targetCataInfo.get("AttributeDesc"))) {
            boolean sourceStrictDataMode = false;
            boolean targetStrictDataMode = false;
            boolean sourceAutoIdx = true;
            boolean targetAutoIdx = true;
            if (sourceCataInfo.containsField("AttributeDesc")) {
                String attributeDesc = BsonUtils.getString(sourceCataInfo, "AttributeDesc");
                if (attributeDesc.contains("StrictDataMode")) {
                    sourceStrictDataMode = true;
                }
                if (attributeDesc.contains("NoIDIndex")) {
                    sourceAutoIdx = false;
                }
            }

            if (targetCataInfo.containsField("AttributeDesc")) {
                String attributeDesc = BsonUtils.getString(targetCataInfo, "AttributeDesc");
                if (attributeDesc.contains("StrictDataMode")) {
                    targetStrictDataMode = true;
                }
                if (attributeDesc.contains("NoIDIndex")) {
                    targetAutoIdx = false;
                }
            }

            if (sourceStrictDataMode != targetStrictDataMode) {
                newOption.put("StrictDataMode", sourceStrictDataMode);
            }

            if (sourceAutoIdx != targetAutoIdx) {
                newOption.put("AutoIndexId", sourceAutoIdx);
            }
        }

        if (newOption.isEmpty()) {
            return null;
        }
        return newOption;
    }

    protected boolean attachInfoIsSame(BSONObject sAttachInfo, BSONObject tAttachInfo) {
        BSONObject sLowBound = BsonUtils.getBSONChecked(sAttachInfo, "LowBound");
        BSONObject sUpBound = BsonUtils.getBSONChecked(sAttachInfo, "UpBound");
        BSONObject tLowBound = BsonUtils.getBSONChecked(tAttachInfo, "LowBound");
        BSONObject tUpBound = BsonUtils.getBSONChecked(tAttachInfo, "UpBound");
        return sLowBound.equals(tLowBound) && sUpBound.equals(tUpBound);
    }

    private boolean isDiff(Object l, Object r) {
        if (l == null && r == null) {
            return false;
        }

        if (l == null || r == null) {
            return true;
        }

        return !l.equals(r);
    }

    @Override
    public void unlockTransactionLock() {
        if (transactionLock != null) {
            transactionLock.unlock();
            transactionLock = null;
        }
    }

    public String getSourceSite() {
        return sourceSite;
    }

    public String getTargetSite() {
        return targetSite;
    }

    @Override
    public void _stop() {
        running = false;
    }

    @Override
    public String getTaskId() {
        return taskId;
    }

    @Override
    public TaskEntity getTaskInfo() {
        return taskEntity;
    }

    public abstract void doTask() throws Exception;

    public BSONObject getContent() {
        return taskEntity.getContent();
    }

    public String getDatasourceName() {
        return datasourceName;
    }

    class InterruptionFlag {
        private boolean interrupted = false;

        public InterruptionFlag() {
        }

        public boolean isInterrupted() {
            return interrupted;
        }

        public void markInterrupted() {
            this.interrupted = true;
        }
    }

    class SaveCLSnapshotPreHandleRes {
        private boolean clTruncated = false;
        public SaveCLSnapshotPreHandleRes(boolean clTruncated) {
            this.clTruncated = clTruncated;
        }

        public boolean isClTruncated() {
            return clTruncated;
        }
    }
}
