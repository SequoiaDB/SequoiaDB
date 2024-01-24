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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.config.SplitMode;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.source.split.SDBSplit;
import com.sequoiadb.flink.common.util.SDBInfoUtil;

import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * A strategy interface only for generating SDBSplit, it can generate different
 * types of SDBSplit via different implementations.
 *
 * like {@link SDBShardingSplitStrategy},{@link SDBDataBlockSplitStrategy}.
 */
public interface SDBSplitStrategy {

    List<SDBSplit> computeSplits();

    /**
     * shuffle the generated splits to ensure that SDBSplits of the
     * same node are not arranged next to each other.
     */
    default List<SDBSplit> shuffleSplits(List<SDBSplit> generatedSplits) {
        List<List<SDBSplit>> curr = new ArrayList<>();

        Map<String, List<SDBSplit>> urlToSplits
                = generatedSplits.stream().collect(Collectors.groupingBy(split -> split.getUrls().get(0)));

        int minSplitsPerNode = Integer.MAX_VALUE;
        int maxSplitsPerNode = Integer.MIN_VALUE;
        for (List<SDBSplit> splits : urlToSplits.values()) {
            curr.add(splits);
            minSplitsPerNode = Math.min(minSplitsPerNode, splits.size());
            maxSplitsPerNode = Math.max(maxSplitsPerNode, splits.size());
        }

        List<SDBSplit> result = new ArrayList<>();
        for (int i = 0; i < maxSplitsPerNode; i++) {
            for (List<SDBSplit> splits : curr) {
                if (!splits.isEmpty()) {
                    result.add(splits.remove(0));
                }
            }
        }
        return result;
    }

    static StrategyBuilder builder() {
        return new StrategyBuilder();
    }

    /**
     * StrategyBuilder is used to create a SDBSplitStrategy through the option
     * {@splitmode} specified by user.
     */
    class StrategyBuilder {

        private static final Logger LOG = LoggerFactory.getLogger(StrategyBuilder.class);

        private SDBSourceOptions sourceOptions;
        private BSONObject matcher;
        private BSONObject selector;
        private long limit;

        public StrategyBuilder options(SDBSourceOptions sourceOptions) {
            this.sourceOptions = sourceOptions;
            return this;
        }

        public StrategyBuilder matcher(BSONObject matcher) {
            this.matcher = matcher;
            return this;
        }

        public StrategyBuilder selector(BSONObject selector) {
            this.selector = selector;
            return this;
        }

        public StrategyBuilder limit(long limit) {
            this.limit = limit;
            return this;
        }

        public SDBSplitStrategy build() {
            SplitMode splitMode = determineSplitMode(sourceOptions.getSplitMode());

            LOG.info("split mode: {}", splitMode);
            switch (splitMode) {
                case DATA_BLOCK:    return new SDBDataBlockSplitStrategy(sourceOptions, limit);
                case SHARDING:      return new SDBShardingSplitStrategy(sourceOptions, matcher, selector);
                default:
                    throw new SDBException(String.format("unknown split mode: %s.", splitMode));
            }
        }

        /**
         * determine which split mode to be used.
         */
        private SplitMode determineSplitMode(SplitMode splitMode) {
            switch (splitMode) {
                case AUTO:          return autoDetectSplitMode();
                case DATA_BLOCK:    return SplitMode.DATA_BLOCK;
                case SHARDING:      return SplitMode.SHARDING;
                default:
                    throw new SDBException(String.format("unknown split mode: %s.", splitMode));
            }
        }

        /**
         * auto deduct split mode via scanType. If one sharding can
         * use index to scan, using sharding mode, and others using
         * datablock mode.
         */
        private SplitMode autoDetectSplitMode() {
            try (Sequoiadb sdb = new Sequoiadb(sourceOptions.getHosts(),
                    sourceOptions.getUsername(), sourceOptions.getPassword(), new ConfigOptions())) {
                List<ShardingInfo> shardingInfos = SDBInfoUtil.getShardingInfos(sdb, sourceOptions, matcher, selector);
                for (ShardingInfo shardingInfo : shardingInfos) {
                    if ("ixscan".equals(shardingInfo.scanType)) {
                        return SplitMode.SHARDING;
                    }
                }
            }

            return SplitMode.DATA_BLOCK;
        }

    }

}
