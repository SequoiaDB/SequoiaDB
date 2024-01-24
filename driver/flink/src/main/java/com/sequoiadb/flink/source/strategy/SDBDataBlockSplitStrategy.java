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

package com.sequoiadb.flink.source.strategy;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.config.SplitMode;
import com.sequoiadb.flink.common.constant.SDBConstant;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.source.split.SDBSplit;
import com.sequoiadb.flink.common.util.RetryUtil;
import com.sequoiadb.flink.common.util.SDBInfoUtil;

import org.apache.flink.calcite.shaded.com.google.common.collect.Lists;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class SDBDataBlockSplitStrategy implements SDBSplitStrategy {

    private static final Logger LOG = LoggerFactory.getLogger(SDBDataBlockSplitStrategy.class);
    private static final int MAX_RETRY_TIMES = 3;
    private static final int INITIAL_RETRY_DURATION = 50;

    private final SDBSourceOptions sourceOptions;

    private final long limit;

    public SDBDataBlockSplitStrategy(SDBSourceOptions sourceOptions,
                                     long limit) {
        this.sourceOptions = sourceOptions;
        this.limit = limit;
    }

    @Override
    public List<SDBSplit> computeSplits() {
        Map<String, List<NodeInfo>> dataGroups;
        List<ShardingInfo> shardingInfos;

        LOG.info("PreferredInstance: {}", sourceOptions.getPreferredInstance());

        try (Sequoiadb sdb = new Sequoiadb(sourceOptions.getHosts(),
                sourceOptions.getUsername(), sourceOptions.getPassword(), new ConfigOptions())) {
            dataGroups = SDBInfoUtil.getDataGroups(sdb);
            if (dataGroups.isEmpty()) {
                throw new SDBException(String.format("SequoiaDB has no normal nodes, coord nodes: [%s].",
                        sourceOptions.getHosts()));
            }
            shardingInfos = SDBInfoUtil.getShardingInfos(sdb, sourceOptions, null, null);
        }

        List<QueryMeta> queryMetas = getQueryMetas(dataGroups, shardingInfos);

        return shuffleSplits(generateSplits(queryMetas));
    }

    /**
     * generate SDBSplit by QueryMetas
     *
     * @param queryMetas query metas including datablocks
     * @return generated SDBSplits
     */
    private List<SDBSplit> generateSplits(List<QueryMeta> queryMetas) {
        List<SDBSplit> splits = new ArrayList<>();

        // group query meta via node's url
        Map<String, List<QueryMeta>> urlToQueryMetas
                = queryMetas.stream().collect(Collectors.groupingBy(queryMeta -> queryMeta.url));

        // foreach different node's queryMetas
        urlToQueryMetas.forEach((url, queryMetaList) -> {
            // for query meta, cur its internal datablocks to the specified size.
            for (QueryMeta queryMeta : queryMetaList) {
                if (!queryMeta.dataBlocks.isEmpty()) {
                    Lists.partition(queryMeta.dataBlocks, sourceOptions.getSplitBlockNum())
                            .forEach(dataBlocks -> {
                                splits.add(new SDBSplit(
                                        Lists.newArrayList(url),
                                        queryMeta.csName,
                                        queryMeta.clName,
                                        SplitMode.DATA_BLOCK,
                                        dataBlocks
                                        ));
                            });
                }
            }
        });

        return splits;
    }

    private List<QueryMeta> getQueryMetas(Map<String, List<NodeInfo>> dataGroups,
                                          List<ShardingInfo> shardingInfos) {
        List<QueryMeta> queryMetas = new ArrayList<>();
        NodeSelector selector = new NodeSelector();

        for (ShardingInfo shardingInfo : shardingInfos) {
            List<NodeInfo> candidateNodes = dataGroups.get(shardingInfo.groupName);

            List<QueryMeta> queryMeta = null;
            if ("".equals(shardingInfo.groupName)) {
                queryMeta = getQueryMeta(shardingInfo.nodeName, shardingInfo.csName,
                        shardingInfo.clName, sourceOptions);
            } else {
                queryMeta = getQueryMeta(candidateNodes, selector, shardingInfo, sourceOptions);
            }

            if (!queryMeta.isEmpty()) {
                queryMetas.addAll(queryMeta);
            }
        }

        return queryMetas;
    }

    // get query meta of collection on specified data node.
    private List<QueryMeta> getQueryMeta(String url, String csName, String clName, SDBSourceOptions sourceOptions) {
        List<QueryMeta> queryMeta = new ArrayList<>();

        Sequoiadb sdb = new Sequoiadb(url,
                sourceOptions.getUsername(), sourceOptions.getPassword(), new ConfigOptions());
        DBCursor cursor = null;
        try {
            DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);

            String firstHostName = null;
            String firstSvcName = null;
            List<Integer> blocks = new ArrayList<>();

            // get query meta via driver's interface.
            cursor = cl.getQueryMeta(null, null, null, 0, limit, 0);
            while (cursor.hasNext()) {
                BSONObject bsonObj = cursor.getNext();

                String hostName = (String) bsonObj.get(SDBConstant.HOSTNAME);
                String svcName = (String) bsonObj.get(SDBConstant.SERVICE_NAME);
                if (firstHostName == null) {
                    firstHostName = hostName;
                    firstSvcName = svcName;
                } else {
                    if (!firstHostName.equals(hostName) || !firstSvcName.equals(svcName)) {
                        throw new SDBException(
                                String.format("node is mismatch: [%s:%s, %s:%s].",
                                        firstHostName, firstSvcName, hostName, svcName));
                    }
                }

                BasicBSONList datablocks = (BasicBSONList) bsonObj.get(SDBConstant.DATA_BLOCKS);
                for (Object blockId : datablocks) {
                    blocks.add((Integer) blockId);
                }
            }

            if (blocks.size() > 0) {
                QueryMeta meta = new QueryMeta(
                        firstHostName + ":" + firstSvcName,
                        csName,
                        clName,
                        blocks);
                queryMeta.add(meta);
            }
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                throw new SDBException(
                        String.format("collection space %s does not exist on node: %s.",
                                sourceOptions.getCollectionSpace(), sdb.getNodeName()));
            } else if (ex.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                throw new SDBException(
                        String.format("collection %s does not exist on node: %s.",
                                sourceOptions.getCollection(), sdb.getNodeName()));
            }
            throw ex;
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            sdb.close();
        }

        return queryMeta;
    }

    /**
     * when failed to get query meta on selected data node (maybe
     * cs/cl is not exists or disk error), try other nodes in the
     * same group.
     */
    private List<QueryMeta> getQueryMeta(List<NodeInfo> candidateNodes, NodeSelector selector,
            ShardingInfo shardingInfo, SDBSourceOptions sourceOptions) {
        NodeInfo nodeInfo = null;
        String url = null;
        SDBSourceOptions.PreferredInstance preferredInstance
                = sourceOptions.getPreferredInstance();

        if ("".equals(shardingInfo.groupName)) {
            url = shardingInfo.nodeName;
        } else {
            nodeInfo = selector.select(candidateNodes, preferredInstance);
            url = nodeInfo.url;
        }

        List<QueryMeta> queryMeta = null;
        List<String> exceptionMessages = new ArrayList<>();
        try {
            queryMeta = getQueryMeta(url, shardingInfo.csName, shardingInfo.clName, sourceOptions);
        } catch (Throwable ex) {
            exceptionMessages.add(ex.getMessage());

            final String fUrl = url;
            // quickly retry current node when failed to solve the
            // problem that cs/cl synchronization may not be timely
            // due to the unstable network.
            queryMeta = RetryUtil.retryWhenRuntimeException(
                    () -> getQueryMeta(fUrl, shardingInfo.csName, shardingInfo.clName, sourceOptions),
                    MAX_RETRY_TIMES,
                    INITIAL_RETRY_DURATION,
                    false);
        }

        if (queryMeta != null) {
            return queryMeta;
        }

        candidateNodes.remove(nodeInfo);
        // after a failed retry under the specified policy, the node
        // may have a more serious problem (like disk error). so try
        // other nodes in the same replica group.
        while (!candidateNodes.isEmpty() &&
               (nodeInfo = selector.select(candidateNodes, preferredInstance)) != null) {
            try {
                queryMeta = getQueryMeta(nodeInfo.url, shardingInfo.csName, shardingInfo.clName, sourceOptions);
                return queryMeta;
            } catch (Throwable ex) {
                exceptionMessages.add(ex.getMessage());

                candidateNodes.remove(nodeInfo);
            }
        }

        // if still failed, throw exception to prompt user to check
        // the replica group.
        throw new SDBException(
                String.format("failed to get query meta, group %s has no normal nodes, please check replicas group's " +
                        "health status.\n" +
                        "exceptions on each retry:\n" +
                        "%s", shardingInfo.groupName, String.join("\n", exceptionMessages)));
    }

    private static class QueryMeta {
        final String url;
        final String csName;
        final String clName;
        final List<Integer> dataBlocks;

        public QueryMeta(String url, String csName, String clName, List<Integer> dataBlocks) {
            this.url = url;
            this.csName = csName;
            this.clName = clName;
            this.dataBlocks = dataBlocks;
        }

        @Override
        public String toString() {
            return "QueryMeta{" +
                    "url='" + url + '\'' +
                    ", csName='" + csName + '\'' +
                    ", clName='" + clName + '\'' +
                    ", dataBlocks=" + dataBlocks +
                    '}';
        }
    }

}
