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

package com.sequoiadb.flink.source.enumerator;

import java.io.IOException;
import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.source.split.SDBSplit;
import com.sequoiadb.flink.source.strategy.SDBSplitStrategy;

import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.source.SplitEnumerator;
import org.apache.flink.api.connector.source.SplitEnumeratorContext;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * SDBSplitEnumerator implements SplitEnumerator, is responsible for the followings:
 *  1. discover and compute the splits for the SDBReader to read;
 *  2. assign the splits to SDBReader.
 */
public class SDBSplitEnumerator implements SplitEnumerator<SDBSplit, List<SDBSplit>> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBSplitEnumerator.class);

    private final SplitEnumeratorContext<SDBSplit> context;
    private final SDBSourceOptions sourceOptions;

    private final List<SDBSplit> pendingSplits = Lists.newArrayList();

    private final BSONObject selector;
    private final BSONObject matcher;
    private long limit = -1;

    public SDBSplitEnumerator(SplitEnumeratorContext<SDBSplit> context,
                              SDBSourceOptions sourceOptions,
                              BSONObject matcher,
                              BSONObject selector,
                              long limit) {
        this(context, Collections.emptyList(), sourceOptions, matcher, selector, limit);
    }

    public SDBSplitEnumerator(SplitEnumeratorContext<SDBSplit> context,
                              List<SDBSplit> splits,
                              SDBSourceOptions sourceOptions,
                              BSONObject matcher,
                              BSONObject selector,
                              long limit) {
        this.context = context;
        this.sourceOptions = sourceOptions;
        pendingSplits.addAll(splits);

        this.matcher = matcher;
        this.selector = selector;
        this.limit = limit;
    }

    /**
     * compute the splits via SDBSplitStrategy, currently has two implementations,
     * SDBShardingSplitStrategy and SDBDatablockStrategy which correspond to
     * two split mode: sharding, datablock.
     */
    @Override
    public void start() {
        LOG.info("SDBSplitEnumerator started, computing splits...");

        SDBSplitStrategy strategy = SDBSplitStrategy.builder()
                .matcher(matcher)
                .selector(selector)
                .limit(limit)
                .options(sourceOptions)
                .build();
        pendingSplits.addAll(strategy.computeSplits());

        pendingSplits.forEach(pendingSplit -> LOG.info("split: " + pendingSplit));
    }

    /**
     * It's a callback, triggered when SDBReader send split request via
     * SourceReaderContext, it will just assign a split to SDBReader when
     * pendingSplits is not empty. When the pendingSplits is exhausted,
     * just send a signal to tell SDBReader there are no more splits
     * via SplitEnumeratorContext.
     *
     * @param subtaskId subtask id for logging
     * @param requesterHostname hostname of requester for logging
     */
    @Override
    public void handleSplitRequest(int subtaskId, @Nullable String requesterHostname) {
        LOG.info("handling request from [subtaskId: {}, hostname: {}].", subtaskId, requesterHostname);

        if (pendingSplits.size() > 0) {
            SDBSplit nextSplit = pendingSplits.remove(0);
            context.assignSplit(nextSplit, subtaskId);
            LOG.info("assigned split {} to subtask {}, remaining splits: {}.", nextSplit, subtaskId,
                    pendingSplits.size());
        } else {
            LOG.info("no more splits can be assigned, signal subtask {}.", subtaskId);
            context.signalNoMoreSplits(subtaskId);
        }
    }

    /**
     * Add splits back to the SDBSplitEnumerator, It will only happen
     * when SDBReader fails and there are still splits assigned to it
     * after the last successful checkpoint.
     *
     * @param splits The splits to add back to the enumerator for reassignment
     * @param subtaskId The id of the subtask to which the returned splits belong
     */
    @Override
    public void addSplitsBack(List<SDBSplit> splits, int subtaskId) {
        LOG.info("received {} split(s) back from subtask {}.", splits.size(), subtaskId);
        pendingSplits.addAll(splits);
    }

    @Override
    public void addReader(int subtaskId) { }

    /**
     * Create a snapshot of the state of SDBSplitEnumerator, to be
     * stored in a checkpoint.
     *
     * @param checkpointId The id of the checkpoint created by Flink
     * @return state of SDBSplitEnumerator
     * @throws Exception when the snapshot cannot be taken
     */
    @Override
    public List<SDBSplit> snapshotState(long checkpointId) throws Exception {
        return pendingSplits;
    }

    @Override
    public void close() throws IOException { }

}
