/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.flink.common.util;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.constant.SDBConstant;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.source.strategy.NodeInfo;
import com.sequoiadb.flink.source.strategy.ShardingInfo;
import org.apache.flink.metrics.MetricGroup;
import org.apache.flink.runtime.metrics.scope.ScopeFormat;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class SDBInfoUtil {

    private static final Logger LOG = LoggerFactory.getLogger(SDBInfoUtil.class);

    private static final String SESSION_ATTR_SOURCE = "Source";
    private static final String SESSION_ATTR_SOURCE_PREFIX = "flink";

    public static Map<String, List<NodeInfo>> getDataGroups(Sequoiadb sdb) {
        Map<String, List<NodeInfo>> dataGroups = new HashMap<>();

        Map<String, String> abnormalNodes = getAbnormalNodes(sdb);
        if (!abnormalNodes.isEmpty()) {
            LOG.info("abnormal nodes of SequoiaDB: {}", abnormalNodes);
        }

        try (DBCursor cursor = sdb.listReplicaGroups()) {
            while (cursor.hasNext()) {
                BSONObject replicaGroupInfo = cursor.getNext();
                String groupName = (String) replicaGroupInfo.get(SDBConstant.GROUP_NAME);

                if (!SDBConstant.COORD_GROUP.equals(groupName) && !SDBConstant.CATALOG_GROUP.equals(groupName)) {
                    List<NodeInfo> nodeInfos = new ArrayList<>();

                    int primaryNodeId = -1;
                    if (replicaGroupInfo.containsField(SDBConstant.PRIMARY_NODE)) {
                        primaryNodeId = (int) replicaGroupInfo.get(SDBConstant.PRIMARY_NODE);
                    }
                    BasicBSONList group = (BasicBSONList) replicaGroupInfo.get(SDBConstant.GROUP);

                    int id = 0;
                    for (Object node : group) {
                        id += 1;
                        BSONObject nodeObj = (BSONObject) node;
                        int status = (int) nodeObj.get(SDBConstant.NODE_STATUS);
                        if (status == 1) {
                            String hostName = (String) nodeObj.get(SDBConstant.HOSTNAME);
                            BasicBSONList service = (BasicBSONList) nodeObj.get(SDBConstant.SERVICE);
                            int nodeId = (int) nodeObj.get(SDBConstant.NODE_ID);

                            int instanceid = id;
                            if (nodeObj.containsField(SDBConstant.INSTANCE_ID)) {
                                instanceid = (int) nodeObj.get(SDBConstant.INSTANCE_ID);
                            }

                            for (Object port : service) {
                                BSONObject portObj = (BSONObject) port;
                                int serviceType = (int) portObj.get(SDBConstant.SERVICE_TYPE);
                                if (serviceType == 0) {
                                    String serviceName = (String) portObj.get(SDBConstant.NAME);
                                    String nodeName = hostName + ":" + serviceName;
                                    if (abnormalNodes.isEmpty() || !abnormalNodes.containsKey(nodeName)) {
                                        NodeInfo nodeInfo = new NodeInfo(
                                                groupName,
                                                hostName,
                                                Integer.parseInt(serviceName),
                                                instanceid,
                                                nodeId == primaryNodeId
                                        );
                                        nodeInfos.add(nodeInfo);
                                        LOG.info("find node: {}", nodeInfo);
                                    }
                                }
                            }
                        }
                    }

                    if (!nodeInfos.isEmpty()) {
                        dataGroups.put(groupName, nodeInfos);
                    }
                }
            }
        } catch (BaseException ex) {
            if (ex.getErrorCode() != SDBError.SDB_RTN_COORD_ONLY.getErrorCode()) {
                throw ex;
            }
        }

        return dataGroups;
    }

    public static List<ShardingInfo> getShardingInfos(Sequoiadb sdb,
                                                      SDBSourceOptions sourceOptions,
                                                      BSONObject matcher,
                                                      BSONObject selector) {
        List<ShardingInfo> shardingInfos = new ArrayList<>();

        DBCursor cursor = null;
        try {
            DBCollection cl = sdb.getCollectionSpace(sourceOptions.getCollectionSpace())
                    .getCollection(sourceOptions.getCollection());

            cursor = cl.explain(matcher, selector, null, null,
                    0, -1, 0, null);

            while (cursor.hasNext()) {
                BSONObject bsonObject = cursor.getNext();

                String groupName = (String) bsonObject.get(SDBConstant.GROUP_NAME);
                String nodeName = (String) bsonObject.get(SDBConstant.NODE_NAME);

                BasicBSONList subCollections = (BasicBSONList) bsonObject.get(SDBConstant.SUB_COLLECTIONS);
                if (subCollections != null) {
                    for (Object subCl : subCollections) {
                        BSONObject subClObj = (BSONObject) subCl;

                        String clFullName = (String) subClObj.get(SDBConstant.CL_FULL_NAME);
                        String scanType = (String) subClObj.get(SDBConstant.SCAN_TYPE);

                        ShardingInfo shardingInfo = new ShardingInfo(
                                groupName,
                                nodeName,
                                scanType,
                                clFullName
                        );

                        shardingInfos.add(shardingInfo);
                    }
                } else {
                    String clFullName = (String) bsonObject.get(SDBConstant.CL_FULL_NAME);
                    String scanType = (String) bsonObject.get(SDBConstant.SCAN_TYPE);

                    ShardingInfo shardingInfo = new ShardingInfo(
                            groupName,
                            nodeName,
                            scanType,
                            clFullName
                    );
                    shardingInfos.add(shardingInfo);
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        return shardingInfos;
    }

    private static Map<String, String> getAbnormalNodes(Sequoiadb sdb) {
        Map<String, String> abnormalNodes = new HashMap<>();

        DBCursor cursor = null;
        try {
            try {
                cursor = sdb.exec("SELECT NodeName, ServiceStatus, Status FROM $SNAPSHOT_SYSTEM");
            } catch (BaseException ex) {
                if (ex.getErrorCode() == SDBError.SDB_INVALIDARG.getErrorCode() ||
                        ex.getErrorCode() == SDBError.SDB_OPTION_NOT_SUPPORT.getErrorCode()) {
                    cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SYSTEM,
                            (BSONObject) null, null, null);
                } else {
                    throw ex;
                }
            }

            while (cursor.hasNext()) {
                BSONObject bsonObj = cursor.getNext();

                BasicBSONList errNodes = (BasicBSONList) bsonObj.get(SDBConstant.ERR_NODES);
                if (errNodes != null && !errNodes.isEmpty()) {
                    errNodes.forEach(errNode -> {
                        BSONObject errNodeObj = (BSONObject) errNode;
                        String nodeName = (String) errNodeObj.get(SDBConstant.NODE_NAME);
                        Integer flag = (Integer) errNodeObj.get(SDBConstant.NODE_FLAG);
                        abnormalNodes.put(nodeName, String.valueOf(flag));
                    });
                } else if (bsonObj.containsField(SDBConstant.SERVICE_STATUS)) {
                    boolean serviceStatus = (boolean) bsonObj.get(SDBConstant.SERVICE_STATUS);
                    if (!serviceStatus) {
                        String nodeName = (String) bsonObj.get(SDBConstant.NODE_NAME);
                        String status = (String) bsonObj.get(SDBConstant.NODE_STATUS);
                        abnormalNodes.put(nodeName, status);
                    }
                }
            }
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_RTN_COORD_ONLY.getErrorCode()) {
                throw ex;
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        return abnormalNodes;
    }

    public static boolean containValidation(BSONObject bsonObject1, BSONObject bsonObject2) {
        Set<String> keySet1 = bsonObject1.keySet();
        Set<String> keySet2 = bsonObject2.keySet();

        return keySet1.containsAll(keySet2);
    }

    /**
     * Set SessionAttr in target Sequoiadb Connection.
     *
     * Notes:
     *   Source SessionAttr may not be supported in some older versions
     *   of SequoiaDB.
     *   Here will just ignore the exception, when trying to set
     *   Source SessionAttr, and print warning log. Mark sure that the
     *   job can continue to be executed.
     *
     * @param sdb
     * @param sourceInfo
     */
    public static void setupSourceSessionAttrIgnoreFailures(Sequoiadb sdb, String sourceInfo) {
        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put(
                SESSION_ATTR_SOURCE,
                String.join("-", new String[]{
                        SESSION_ATTR_SOURCE_PREFIX, sourceInfo}));

        try {
            sdb.setSessionAttr(sessionAttr);
        } catch (BaseException ex) {
            LOG.warn("Failed to set {} session attribute, msg: {}",
                    SESSION_ATTR_SOURCE, ex.getMessage());
        }
    }

    /**
     * generate source info which can help users locate which job
     * on which flink task-manager is talking to SequoiaDB.
     *
     * Source Pattern: flink-${task_manager_id}-${job_id}
     * Example:
     *      flink-hostname:port-b80a46-12efdb12040a6baeb028b455b43bacd7
     *
     * Notes:
     *  if we can not get task manager id, job id from {@link MetricGroup},
     *  it will set noting in source info, and print a warning log.
     *  Make sure that the job can continue to run when the above infos
     *  can not be obtained.
     *
     * @param metricGroup which is holding environments of flink's job.
     * @return
     */
    public static String generateSourceInfo(MetricGroup metricGroup) {
        Map<String, String> allVars = metricGroup
                .getAllVariables();

        String tmId = "";
        String jobId = "";
        if (allVars != null) {
            tmId = allVars.get(ScopeFormat.SCOPE_TASKMANAGER_ID);
            jobId = allVars.get(ScopeFormat.SCOPE_JOB_ID);
        }

        if ("".equals(tmId) || "".equals(jobId)) {
            LOG.warn("Can not obtain task manager id or job id for generating source info.");
        }

        return String.join("-", tmId, jobId);
    }

}
