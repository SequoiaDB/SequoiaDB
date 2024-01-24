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

package com.sequoiadb.flink.config;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.client.SDBSinkClient;
import org.apache.flink.configuration.ReadableConfig;
import org.apache.flink.table.catalog.UniqueConstraint;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;

public class SDBSinkOptions extends SDBClientOptions {

    private final int sinkParallelism;

    private final int bulkSize;

    private final boolean ignoreNullField;

    private final int pageSize;

    private final String domain;

    private final String shardingKey;

    private final String shardingType;

    private final int replSize;

    private final String compressionType;

    private final boolean autoPartition;

    private final String group;

    private final long maxBulkFillTime;

    private boolean overwrite;

    private boolean idempotent;

    private String writeMode;

    private HashSet<String> primaryKey;
    private String[] upsertKey = new String[]{};

    private final String eventTsFieldName;
    private final int stateTtl;
    private final boolean partitionedSource;

    public SDBSinkOptions(ReadableConfig options) {
        super(options);

        this.sinkParallelism = options.get(SDBConfigOptions.SINK_PARALLELISM);
        this.bulkSize = options.get(SDBConfigOptions.BULK_SIZE);
        this.ignoreNullField = options.get(SDBConfigOptions.IGNORE_NULL_FIELD);
        this.pageSize = options.get(SDBConfigOptions.PAGE_SIZE);
        this.domain = options.get(SDBConfigOptions.DOMAIN);
        this.shardingKey = options.get(SDBConfigOptions.SHARDING_KEY);
        this.shardingType = options.get(SDBConfigOptions.SHARDING_TYPE);
        this.replSize = options.get(SDBConfigOptions.REPL_SIZE);
        this.compressionType = options.get(SDBConfigOptions.COMPRESSION_TYPE);
        this.autoPartition = options.get(SDBConfigOptions.AUTO_PARTITION);
        this.group = options.get(SDBConfigOptions.GROUP);
        this.maxBulkFillTime = options.get(SDBConfigOptions.MAX_BULK_FILL_TIME);
        this.overwrite = options.get(SDBConfigOptions.OVERWRITE);

        this.writeMode = options.get(SDBConfigOptions.WRITE_MODE);

        this.eventTsFieldName = options.get(
                SDBConfigOptions.SINK_RETRACT_EVENT_TS_FIELD_NAME);
        this.stateTtl = options.get(
                SDBConfigOptions.SINK_RETRACT_STATE_TTL);
        this.partitionedSource = options.get(
                SDBConfigOptions.SINK_RETRACT_PARTITIONED_SOURCE);
    }

    @Override
    public String toString() {
        return super.toString();
    }

    public String getWriteMode() {
        return writeMode;
    }

    public int getSinkParallelism() {
        return sinkParallelism;
    }

    public int getBulkSize() {
        return bulkSize;
    }

    public boolean getIgnoreNullField() {
        return ignoreNullField;
    }

    public int getPageSize() {
        return pageSize;
    }

    public String getDomain() {
        return domain;
    }

    public String getShardingKey() {
        return shardingKey;
    }

    public String getShardingType() {
        return shardingType;
    }

    public int getReplSize() {
        return replSize;
    }

    public String getCompressionType() {
        return compressionType;
    }

    public boolean getAutoPartition() {
        return autoPartition;
    }

    public String getGroup() {
        return group;
    }

    public long getMaxBulkFillTime() {
        return maxBulkFillTime;
    }

    public void setOverwrite(boolean overwrite) {
        this.overwrite = overwrite;
    }

    public boolean isOverwrite() {
        return overwrite;
    }

    public boolean isIdempotent() {
        return idempotent;
    }

    public String[] getUpsertKey() {
        return upsertKey;
    }

    public void setUpsertKey(String[] upsertKey) {
        this.upsertKey = upsertKey;
    }

    public HashSet<String> getPrimaryKey() {
        return primaryKey;
    }

    public String getEventTsFieldName() {
        return eventTsFieldName;
    }

    public int getStateTtl() {
        return stateTtl;
    }

    public boolean isPartitionedSource() {
        return partitionedSource;
    }

    public void computeIdempotentWriteOptimization(Optional<UniqueConstraint> flinkPrimaryKey) {

        if (flinkPrimaryKey.isPresent()){
            HashSet<String> pks = new HashSet<>(flinkPrimaryKey.get().getColumns());
            List<HashSet<String>> unique_indexes = new ArrayList<>();
            try {
                unique_indexes = SDBSinkClient.checkUniqueIndex(
                    super.getHosts(),
                    super.getCollectionSpace(),
                    super.getCollection(),
                    super.getUsername(),
                    super.getPassword());
            } catch (BaseException e) {
                if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                 || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                    primaryKey = pks;
                } else {
                    throw e;
                }
            }

            boolean hasIndex = false;
            for (HashSet<String> uniquekeyset : unique_indexes){
                if (uniquekeyset.equals(pks)) {
                    hasIndex = true;
                    break;
                }
            }

            this.idempotent = !unique_indexes.isEmpty() && hasIndex || primaryKey != null;
        } else {
            this.idempotent = false;
        }

    }
}
