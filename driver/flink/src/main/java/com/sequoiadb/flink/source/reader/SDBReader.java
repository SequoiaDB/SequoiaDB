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

import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.source.split.SDBSplit;
import org.apache.flink.api.connector.source.SourceReaderContext;
import org.apache.flink.connector.base.source.reader.SingleThreadMultiplexSourceReaderBase;
import org.apache.flink.table.data.RowData;
import org.bson.BSONObject;

import java.util.Map;

/**
 * SDBReader implements {@link SingleThreadMultiplexSourceReaderBase}, it means that SDBReader
 * read splits with one thread using one {@link SDBSplitReader}.
 */
public class SDBReader extends SingleThreadMultiplexSourceReaderBase<byte[], RowData, SDBSplit, SDBSplit> {

    public SDBReader(SourceReaderContext context,
                     SDBDataConverter dataConverter,
                     SDBSourceOptions sourceOptions,
                     BSONObject matcher,
                     BSONObject selector,
                     long limit) {
        super(
                // supply a SDBSplitReader (for reading split)
                () -> {
                    sourceOptions.setSourceInfo(
                            SDBInfoUtil.generateSourceInfo(context.metricGroup()));
                    return new SDBSplitReader(sourceOptions, matcher, selector, limit);
                },
                // supply a SDBEmitter (for emitting data to next Operator)
                new SDBEmitter(dataConverter),
                context.getConfiguration(),
                context
                );
    }

    /**
     * Send a request to obtain SDBSplit for reading.
     */
    @Override
    public void start() {
        context.sendSplitRequest();
    }

    /**
     * Handle the finished splits to clean state if needed. currently,
     * just send a request to obtain another {@link SDBSplit}.
     *
     * @param finishedSplitIds finished splits with split id.
     */
    @Override
    protected void onSplitFinished(Map<String, SDBSplit> finishedSplitIds) {
        context.sendSplitRequest();
    }

    /**
     * When new splits are added to SDBReader, initialize the state of
     * new splits. For batch read, split is stateless, so just return
     * itself.
     */
    @Override
    protected SDBSplit initializedState(SDBSplit split) {
        return split;
    }

    /**
     * Convert a mutable split state to SDBSplit, because SDBSplit is
     * stateless, just return itself.
     */
    @Override
    protected SDBSplit toSplitType(String splitId, SDBSplit split) {
        return split;
    }

}
