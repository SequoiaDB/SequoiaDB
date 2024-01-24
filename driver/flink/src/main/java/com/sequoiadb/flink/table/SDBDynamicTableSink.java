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

package com.sequoiadb.flink.table;

import com.sequoiadb.flink.common.client.SDBCollectionProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.sink.SDBPartitionedSink;
import com.sequoiadb.flink.sink.SDBRetractSink;
import com.sequoiadb.flink.sink.SDBSink;
import com.sequoiadb.flink.sink.SDBUpsertSink;
import org.apache.flink.table.catalog.ResolvedSchema;
import org.apache.flink.table.catalog.UniqueConstraint;
import org.apache.flink.table.connector.ChangelogMode;
import org.apache.flink.table.connector.sink.DynamicTableSink;
import org.apache.flink.table.connector.sink.SinkProvider;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class SDBDynamicTableSink implements DynamicTableSink {

    private static final Logger LOG = LoggerFactory.getLogger(SDBDynamicTableSink.class);

    // sink write mode
    public static final String APPEND_ONLY = "append-only";
    public static final String UPSERT = "upsert";
    public static final String RETRACT = "retract";

    private final DataType physicalDataType;

    private final ResolvedSchema schema;

    private final SDBSinkOptions sinkOptions;

    public SDBDynamicTableSink(SDBSinkOptions sinkOptions, ResolvedSchema resolvedSchema) {
        this.sinkOptions = sinkOptions;
        this.schema = resolvedSchema;
        this.physicalDataType = resolvedSchema.toPhysicalRowDataType();
    }

    /**
     * determine changelog mode by writemode (connector option)
     *  1. insertOnly mode only supports INSERT
     *  2. upsert mode supports INSERT, UPDATE_AFTER, DELETE, which means it does not
     *  support change primary key.
     *  3. all mode support INSERT, UPDATE_BEFORE, UPDATE_AFTER, DELETE
     *
     * @param requestedMode expected set of changes by the current plan
     * @return
     */
    @Override
    public ChangelogMode getChangelogMode(ChangelogMode requestedMode) {
        return ChangelogMode.all();
    }

    @Override
    public SinkRuntimeProvider getSinkRuntimeProvider(Context context) {
        LOG.debug("create sink runtime provider");
        SDBDataConverter converter = new SDBDataConverter((RowType) physicalDataType.getLogicalType());

        // check and set upsert key (primary key) to sink options
        // for upsert, retract mode (streaming scenario)
        if (schema.getPrimaryKey().isPresent()) {
            UniqueConstraint uc = schema.getPrimaryKey().get();
            sinkOptions.setUpsertKey(uc.getColumns()
                    .toArray(new String[0]));

            //create collectionspace,collection and index if not exist.
            SDBCollectionProvider.ensureCollectionSpaceWithCollection(sinkOptions);
        }

        if (APPEND_ONLY.equals(sinkOptions.getWriteMode())) {
            return SinkProvider.of(
                    new SDBSink<>(sinkOptions, converter), sinkOptions.getSinkParallelism());
        } else if (RETRACT.equals(sinkOptions.getWriteMode())) {
            if (sinkOptions.isPartitionedSource()) {
                return SinkProvider.of(new SDBPartitionedSink(converter, sinkOptions),
                        sinkOptions.getSinkParallelism());
            }
            return SinkProvider.of(
                    new SDBRetractSink(converter, sinkOptions),
                    sinkOptions.getSinkParallelism());
        } else if (UPSERT.equals(sinkOptions.getWriteMode())) {
            return SinkProvider.of(
                    new SDBUpsertSink(converter, sinkOptions), sinkOptions.getSinkParallelism());
        } else {
            throw new SDBException(String.format(
                    "unsupported write mode: %s", sinkOptions.getWriteMode()));
        }
    }

    @Override
    public DynamicTableSink copy() {
        return new SDBDynamicTableSink(sinkOptions, schema);
    }

    @Override
    public String asSummaryString() {
        return "SequoiaDB Dynamic Table Sink";
    }

}