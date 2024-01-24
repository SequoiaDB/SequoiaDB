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

package com.sequoiadb.flink.source;

import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.source.enumerator.SDBSplitEnumerator;
import com.sequoiadb.flink.source.reader.SDBReader;
import com.sequoiadb.flink.source.split.SDBSplit;
import com.sequoiadb.flink.source.split.SDBSplitListSerializer;
import com.sequoiadb.flink.source.split.SDBSplitSerializer;

import org.apache.flink.api.connector.source.Boundedness;
import org.apache.flink.api.connector.source.Source;
import org.apache.flink.api.connector.source.SourceReader;
import org.apache.flink.api.connector.source.SourceReaderContext;
import org.apache.flink.api.connector.source.SplitEnumerator;
import org.apache.flink.api.connector.source.SplitEnumeratorContext;
import org.apache.flink.core.io.SimpleVersionedSerializer;
import org.apache.flink.table.data.RowData;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

public class SDBSource implements Source<RowData, SDBSplit, List<SDBSplit>> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBSource.class);

    private final SDBDataConverter dataConverter;
    private final SDBSourceOptions sourceOptions;

    private final BSONObject selector = new BasicBSONObject();
    private final long limit;
    private final BSONObject matcher;

    public SDBSource(SDBDataConverter dataConverter, SDBSourceOptions sourceOptions, List<String> requiredColumns,
                     BSONObject matcher, long limit) {
        this.dataConverter = dataConverter;
        this.sourceOptions = sourceOptions;


        requiredColumns.forEach(requiredColumn
                -> selector.put(requiredColumn, null));
        this.matcher = matcher;
        this.limit = limit;

        LOG.info("matcher: {}, selector: {}, limit: {}", matcher, selector, limit);
    }

    @Override
    public Boundedness getBoundedness() {
        return Boundedness.BOUNDED;
    }

    @Override
    public SourceReader<RowData, SDBSplit> createReader(SourceReaderContext readerContext) throws Exception {
        return new SDBReader(readerContext, dataConverter, sourceOptions, matcher, selector, limit);
    }

    @Override
    public SplitEnumerator<SDBSplit, List<SDBSplit>> createEnumerator(SplitEnumeratorContext<SDBSplit> enumContext)
            throws Exception {
        return new SDBSplitEnumerator(enumContext, sourceOptions, matcher, selector, limit);
    }

    @Override
    public SplitEnumerator<SDBSplit, List<SDBSplit>> restoreEnumerator(SplitEnumeratorContext<SDBSplit> enumContext,
                                                                       List<SDBSplit> checkpoint)
            throws Exception {
        return new SDBSplitEnumerator(enumContext, checkpoint, sourceOptions, matcher, selector, limit);
    }

    @Override
    public SimpleVersionedSerializer<SDBSplit> getSplitSerializer() {
        return new SDBSplitSerializer();
    }

    @Override
    public SimpleVersionedSerializer<List<SDBSplit>> getEnumeratorCheckpointSerializer() {
        return new SDBSplitListSerializer();
    }

}
