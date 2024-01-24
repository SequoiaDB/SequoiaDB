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

package com.sequoiadb.flink.source.reader;

import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.source.iterator.SDBIterator;
import com.sequoiadb.flink.source.split.SDBRecords;
import com.sequoiadb.flink.source.split.SDBSplit;
import org.apache.commons.compress.utils.Lists;
import org.apache.flink.calcite.shaded.com.google.common.collect.Queues;
import org.apache.flink.connector.base.source.reader.RecordsWithSplitIds;
import org.apache.flink.connector.base.source.reader.splitreader.SplitReader;
import org.apache.flink.connector.base.source.reader.splitreader.SplitsAddition;
import org.apache.flink.connector.base.source.reader.splitreader.SplitsChange;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;
import java.util.Queue;

/**
 * SDBSplitReader is used to read records from {@link SDBSplit}.
 */
public class SDBSplitReader implements SplitReader<byte[], SDBSplit> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBSplitReader.class);
    private static final int DEFAULT_FETCH_SIZE = 10000; // micro-batch

    private final SDBSourceOptions sourceOptions;

    // current state when reading a split.
    private SDBSplit currentSplit;
    private SDBIterator currentIterator;

    private final Queue<SDBSplit> pendingSplits;

    private final BSONObject selector;
    private final long limit;

    private int offset = 0;

    private BSONObject matcher;

    public SDBSplitReader(SDBSourceOptions sourceOptions, BSONObject matcher, BSONObject selector, long limit) {
        this.sourceOptions = sourceOptions;
        pendingSplits = Queues.newArrayDeque();

        this.matcher = matcher;
        this.selector = selector;
        this.limit = limit;
    }

    @Override
    public RecordsWithSplitIds<byte[]> fetch() throws IOException {
        // prepareFetch is try to initialize currentIterator and currentSplit
        // throws IOException when there is no more splits.
        prepareFetch();

        List<byte[]> rawRecords = Lists.newArrayList();
        while (rawRecords.size() < DEFAULT_FETCH_SIZE && currentIterator.hasNext()) {
            rawRecords.add(currentIterator.next());
        }
        offset += rawRecords.size();

        if (currentIterator.hasNext()) {
            LOG.debug("fetched {} records from split {}, current offset: {}", rawRecords.size(),
                    currentSplit.splitId(), offset);
            // create a SDBRecords without finished split ids.
            return SDBRecords.forRecords(currentSplit.splitId(), rawRecords);
        } else {
            String splitId = currentSplit.splitId();
            closeCurrentIterator();
            // create a SDBRecords with finished split ids.
            return SDBRecords.finishedSplits(splitId, rawRecords);
        }
    }

    /**
     * Handle the splits changes, triggered when new splits added to
     * SDBSplitReader.
     *
     * @param splitsChanges the newly splits that SDBSplitReader
     *                      need to handle
     */
    @Override
    public void handleSplitsChanges(SplitsChange<SDBSplit> splitsChanges) {
        if (!(splitsChanges instanceof SplitsAddition)) {
            throw new UnsupportedOperationException(
                    String.format(
                            "The SplitsChanges type of %s is not supported.",
                            splitsChanges.getClass()));
        }
        LOG.info("Handling splits changes {}", splitsChanges);
        pendingSplits.addAll(splitsChanges.splits());
    }

    @Override
    public void wakeUp() { }

    @Override
    public void close() throws Exception {
        // release resources when SDBSplitReader are closed.
        if (currentIterator != null) {
            currentIterator.close();
        }
    }

    /**
     * obtain a new split to handle, and initialize iterator for
     * reading. If currentSplit is not null, means still records
     * have not been read, just return.
     *
     * @throws IOException throws when there is no more splits need
     *                     to handle
     */
    private void prepareFetch() throws IOException {
        if (currentIterator != null) {
            return;
        }

        currentSplit = pendingSplits.poll();
        if (currentSplit == null) {
            throw new IOException("No more splits for current SplitReader.");
        }

        offset = 0;
        currentIterator = new SDBIterator(currentSplit, sourceOptions, matcher, selector, limit);
    }

    /**
     * close current SDBIterator when there are no more records.
     * It also reset the following states of SDBSplitReader:
     * 1. currentIterator
     * 2. reading offset of currentIterator
     * 3. currentSplit
     */
    private void closeCurrentIterator() throws IOException {
        if (currentIterator != null) {
            currentIterator.close();
            currentIterator = null;
        }

        offset = 0;

        LOG.info("Finished reading split {}.", currentSplit);
        currentSplit = null;
    }

}
