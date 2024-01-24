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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.config.SplitMode;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.source.split.SDBSplit;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * SDBSplit generated strategy for sharding mode.
 */
public class SDBShardingSplitStrategy implements SDBSplitStrategy {

    private static final Logger LOG = LoggerFactory.getLogger(SDBShardingSplitStrategy.class);

    private final SDBSourceOptions sourceOptions;
    private final BSONObject matcher;
    private final BSONObject selector;

    public SDBShardingSplitStrategy(SDBSourceOptions sourceOptions, BSONObject matcher, BSONObject selector) {
        this.sourceOptions = sourceOptions;
        this.matcher = matcher;
        this.selector = selector;
    }

    /**
     * compute SDBSplits in sharding mode.
     * in this mode, one SequoiaDB sharding corresponds to one SDBSplit.
     */
    @Override
    public List<SDBSplit> computeSplits() {
        List<SDBSplit> splits = new ArrayList<>();

        LOG.info("PreferredInstance: {}", sourceOptions.getPreferredInstance());

        try (Sequoiadb sdb = new Sequoiadb(sourceOptions.getHosts(),
                sourceOptions.getUsername(), sourceOptions.getPassword(), new ConfigOptions())) {
            // get replica groups and sharding info via SDBConstant.
            Map<String, List<NodeInfo>> dataGroups = SDBInfoUtil.getDataGroups(sdb);
            List<ShardingInfo> shardingInfos = SDBInfoUtil.getShardingInfos(sdb, sourceOptions, matcher, selector);

            // traverse sharding infos and generate SDBSplits
            for (ShardingInfo shardingInfo : shardingInfos) {
                List<String> urls = new ArrayList<>();
                if ("".equals(shardingInfo.groupName)) { // stand-alone mode
                    urls.add(shardingInfo.nodeName);
                } else {
                    List<NodeInfo> nodeInfos = dataGroups.get(shardingInfo.groupName);
                    if (nodeInfos.isEmpty()) {
                        throw new SDBException(
                                String.format("group %s has no normal nodes, please check replicas group's " +
                                        "health status.\n", shardingInfo.groupName));
                    }
                    urls.addAll(nodeInfos.stream().map(nodeInfo -> nodeInfo.url)
                            .collect(Collectors.toList()));
                }

                splits.add(new SDBSplit(
                        urls,
                        shardingInfo.csName,
                        shardingInfo.clName,
                        SplitMode.SHARDING,
                        null
                        ));
            }
        }

        return shuffleSplits(splits);
    }

}