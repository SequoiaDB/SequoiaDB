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

package com.sequoiadb.flink.sink;

import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.state.EventState;
import com.sequoiadb.flink.sink.state.EventStateSerializer;
import com.sequoiadb.flink.sink.writer.SDBPartitionedSinkWriter;
import org.apache.flink.api.connector.sink.Committer;
import org.apache.flink.api.connector.sink.GlobalCommitter;
import org.apache.flink.api.connector.sink.Sink;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.core.io.SimpleVersionedSerializer;
import org.apache.flink.table.data.RowData;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * SDBPartitionedSink supports consuming changelogs in multi partitions, such as
 * Kafka multi partitions in the topic.
 */
public class SDBPartitionedSink
        implements Sink<RowData, Void, Map<BSONObject, EventState>, Void> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBPartitionedSink.class);

    private SDBDataConverter converter;

    private SDBSinkOptions sinkOptions;

    public SDBPartitionedSink(SDBDataConverter converter, SDBSinkOptions sinkOptions) {
        this.converter = converter;
        this.sinkOptions = sinkOptions;
    }

    /**
     *
     * @param context the runtime context.
     * @param states the writer's previous state.
     * @return
     * @throws IOException
     */
    @Override
    public SinkWriter<RowData, Void, Map<BSONObject, EventState>> createWriter(
            InitContext context,
            List<Map<BSONObject, EventState>> states) throws IOException {
        sinkOptions.setSourceInfo(
                SDBInfoUtil.generateSourceInfo(context.metricGroup()));
        return new SDBPartitionedSinkWriter(converter, sinkOptions, states);
    }

    @Override
    public Optional<SimpleVersionedSerializer<Map<BSONObject, EventState>>> getWriterStateSerializer() {
        return Optional.of(new EventStateSerializer());
    }

    // =============================================================
    // Do not implement these methods which are for 2-PC Committer
    // =============================================================

    @Override
    public Optional<Committer<Void>> createCommitter() throws IOException {
        return Optional.empty();
    }

    @Override
    public Optional<GlobalCommitter<Void, Void>> createGlobalCommitter() throws IOException {
        return Optional.empty();
    }

    @Override
    public Optional<SimpleVersionedSerializer<Void>> getCommittableSerializer() {
        return Optional.empty();
    }

    @Override
    public Optional<SimpleVersionedSerializer<Void>> getGlobalCommittableSerializer() {
        return Optional.empty();
    }
}
