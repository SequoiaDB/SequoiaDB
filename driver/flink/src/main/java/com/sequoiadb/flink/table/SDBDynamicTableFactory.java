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

import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.config.SDBConfigOptions;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.config.SDBSourceOptions;

import org.apache.flink.configuration.ConfigOption;
import org.apache.flink.configuration.CoreOptions;
import org.apache.flink.configuration.ReadableConfig;
import org.apache.flink.table.api.Schema;
import org.apache.flink.table.catalog.ResolvedSchema;
import org.apache.flink.table.catalog.UniqueConstraint;
import org.apache.flink.table.connector.sink.DynamicTableSink;
import org.apache.flink.table.connector.source.DynamicTableSource;
import org.apache.flink.table.factories.DynamicTableSinkFactory;
import org.apache.flink.table.factories.DynamicTableSourceFactory;
import org.apache.flink.table.factories.FactoryUtil;
import org.apache.flink.table.types.DataType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SDBDynamicTableFactory implements DynamicTableSourceFactory, DynamicTableSinkFactory {

    private static final Logger LOG = LoggerFactory.getLogger(SDBDynamicTableFactory.class);

    private static final String IDENTIFIER = "sequoiadb";

    private static final Set<ConfigOption<?>> REQUIRED_OPTIONS =
            new HashSet<>();
    private static final Set<ConfigOption<?>> OPTIONAL_OPTIONS = new HashSet<>();

    static {
        REQUIRED_OPTIONS.add(SDBConfigOptions.HOSTS);
        REQUIRED_OPTIONS.add(SDBConfigOptions.COLLECTION_SPACE);
        REQUIRED_OPTIONS.add(SDBConfigOptions.COLLECTION);

        // sequoiadb client options
        OPTIONAL_OPTIONS.add(SDBConfigOptions.USERNAME);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.PASSWORD_TYPE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.PASSWORD);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.TOKEN);
        // source options
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SPLIT_MODE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SPLIT_BLOCK_NUM);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.PREFERRED_INSTANCE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.PREFERRED_INSTANCE_MODE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.PREFERRED_INSTANCE_STRICT);
        // sink options
        OPTIONAL_OPTIONS.add(SDBConfigOptions.BULK_SIZE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.IGNORE_NULL_FIELD);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.DOMAIN);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.PAGE_SIZE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SHARDING_KEY);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SHARDING_TYPE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.REPL_SIZE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.COMPRESSION_TYPE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.AUTO_PARTITION);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.GROUP);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SINK_PARALLELISM);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.OVERWRITE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.MAX_BULK_FILL_TIME);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.WRITE_MODE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SINK_RETRACT_PARTITIONED_SOURCE);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SINK_RETRACT_EVENT_TS_FIELD_NAME);
        OPTIONAL_OPTIONS.add(SDBConfigOptions.SINK_RETRACT_STATE_TTL);
    }

    @Override
    public String factoryIdentifier() {
        return IDENTIFIER;
    }

    @Override
    public Set<ConfigOption<?>> requiredOptions() {
        return REQUIRED_OPTIONS;
    }

    @Override
    public Set<ConfigOption<?>> optionalOptions() {
        return OPTIONAL_OPTIONS;
    }

    @Override
    public DynamicTableSink createDynamicTableSink(Context context) {
        FactoryUtil.TableFactoryHelper helper = FactoryUtil.createTableFactoryHelper(this, context);
        helper.validate();

        ResolvedSchema resolvedSchema = context.getCatalogTable().getResolvedSchema();

        // check if primary key exist in table schema
        final Optional<UniqueConstraint> pk =
            context.getCatalogTable().getResolvedSchema().getPrimaryKey();

        ReadableConfig options = helper.getOptions();
        SDBSinkOptions sinkOptions = new SDBSinkOptions(options);

        sinkOptions.computeIdempotentWriteOptimization(pk);

        if (!sinkOptions.isOverwrite() && !"append-only".equals(sinkOptions.getWriteMode())) {
            LOG.warn("option 'overwrite' will be ignored on upsert/retract mode.");
            sinkOptions.setOverwrite(true);
        }

        if (sinkOptions.isOverwrite() && !sinkOptions.isIdempotent()) {
            throw new SDBException("can not perform idempotent write when primary key is not specified or " +
                    "SequoiaDB collection does not have a unique index corresponding to flink table primary key.");
        }

        LOG.info("creating sequoiadb dynamic table sink, sink options: {}",
                sinkOptions);

        int parallelism = context
                .getConfiguration()
                .get(CoreOptions.DEFAULT_PARALLELISM);
        String writeMode = sinkOptions.getWriteMode();
        if (SDBDynamicTableSink.RETRACT.equals(writeMode) &&
                sinkOptions.isPartitionedSource()) {
            if (parallelism == sinkOptions.getSinkParallelism()) {
                throw new SDBException(
                        "In retract mode, please ensure that sink parallelism is different from " +
                                "parallelism.default, otherwise the upstream changelog cannot " +
                                "be shuffled by Hash(Primary Key), resulting in an incorrect final result.");
            }
        }

        return new SDBDynamicTableSink(sinkOptions, resolvedSchema);
    }

    //create DynamicTable source
    @Override
    public DynamicTableSource createDynamicTableSource(Context context) {
        FactoryUtil.TableFactoryHelper helper = FactoryUtil.createTableFactoryHelper(this, context);
        helper.validate();

        DataType produceDatatype = context.getCatalogTable()
                .getResolvedSchema()
                .toPhysicalRowDataType();

        ReadableConfig options = helper.getOptions();
        SDBSourceOptions sourceOptions = new SDBSourceOptions(options);

        return new SDBDynamicTableSource(sourceOptions, produceDatatype);
    }

}