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
import com.sequoiadb.flink.sink.committer.SDBCommitter;
import com.sequoiadb.flink.sink.state.SDBBulk;
import com.sequoiadb.flink.sink.state.SDBBulkSerializer;
import com.sequoiadb.flink.sink.writer.SDBSinkWriter;

import org.apache.flink.api.connector.sink.Committer;
import org.apache.flink.api.connector.sink.GlobalCommitter;
import org.apache.flink.api.connector.sink.Sink;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.core.io.SimpleVersionedSerializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;
import java.util.Optional;

// DataStream API SDBSink class implements Sink from flink
public class SDBSink<IN> implements Sink<IN, SDBBulk, SDBBulk, Void> {
    private SDBSinkOptions sdbSinkOptions;
    private final SDBDataConverter dataConverter;
    private final static Logger LOG = LoggerFactory.getLogger(SDBSink.class);

    /*
     * constructor of SDBSink
     * @param sdbSinkOptions        pass down user options
     * @param dataConverter         converter than convert Rowdata to BSON
     */
    public SDBSink(SDBSinkOptions sdbSinkOptions, SDBDataConverter dataConverter) {
        this.sdbSinkOptions = sdbSinkOptions;
        this.dataConverter = dataConverter;
    }
    /*
     * create sink writer  that doing the writes
     * @param context               sink init context
     * @param states                states from last checkpoint, when retry
     * @return                      SinkWriter
     */
    @Override
    public SinkWriter<IN, SDBBulk, SDBBulk> createWriter(InitContext context,
                                                         List<SDBBulk> states) throws IOException {
        LOG.debug("SDBSink creates sink writer");

        sdbSinkOptions.setSourceInfo(
                SDBInfoUtil.generateSourceInfo(context.metricGroup()));
        return new SDBSinkWriter<IN>(sdbSinkOptions, dataConverter, context, states);
    }

    /*
     * create writer state seriallizer, here our state is list of bson we are about to send
     * @return                      Optional SinkWriter
     */
    @Override
    public Optional<SimpleVersionedSerializer<SDBBulk>> getWriterStateSerializer() {
        LOG.debug("SDBSink create writer state serializer");
        return Optional.of(new SDBBulkSerializer());
    }

    /*
    * create committer for sink writers, there will be same number of sinkwriter and committer
    * @return                      Optional Committer
    */
    @Override
    public Optional<Committer<SDBBulk>> createCommitter() throws IOException {
        LOG.debug("SDBSink create writer committer");
        return Optional.of(new SDBCommitter(sdbSinkOptions));
    }

    /*
    * create a global committer, for global committer will commit with parallelism 1
    * @return                      Optional GlobalCommitter
    */
    @Override
    public Optional<GlobalCommitter<SDBBulk, Void>> createGlobalCommitter() throws IOException {
        LOG.debug("SDBSink create writer globalcommitter");
        return Optional.empty();
    }

    /*
    * create a serializer for pass data from sink writer to committer
    * @return                      Optional committer data serializer
    */
    @Override
    public Optional<SimpleVersionedSerializer<SDBBulk>> getCommittableSerializer() {
        LOG.debug("SDBSink create writer committer serializer");
        return Optional.of(new SDBBulkSerializer());
    }

    /*
    * create a serializer for pass data from sink writer to global committer
    * @return                      Optional global committer data serializer
    */
    @Override
    public Optional<SimpleVersionedSerializer<Void>> getGlobalCommittableSerializer() {
        LOG.debug("SDBSink create writer global committer serializer");
        return Optional.empty();
    }

}
