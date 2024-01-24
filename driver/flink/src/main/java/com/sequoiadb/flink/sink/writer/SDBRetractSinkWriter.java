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

package com.sequoiadb.flink.sink.writer;

import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.RetryUtil;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;

import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Stream;

public class SDBRetractSinkWriter implements SinkWriter<RowData, Void, Void> {

    private Set<String> topicSet = new HashSet<>();
    private Set<Integer> partitionSet = new HashSet<>();

    // BSON Modifier Type
    private static final String MODIFIER_SET = "$set";

    // primary key for upsert/delete
    private final String[] upsertKeys;
    private final int[] metadataPositions;

    private final SDBDataConverter converter;
    private final SDBSinkOptions sinkOptions;

    private transient final SDBClientProvider provider;

    public SDBRetractSinkWriter(
            SDBDataConverter converter, SDBSinkOptions sinkOptions) {
        this.upsertKeys = sinkOptions.getUpsertKey();
        RowType rowType = converter.getRowType();
        this.metadataPositions = getMetadataPositions(rowType);

        this.converter = converter;
        this.sinkOptions = sinkOptions;

        this.provider = SDBClientProvider.builder()
                .withHosts(sinkOptions.getHosts())
                .withCollectionSpace(sinkOptions.getCollectionSpace())
                .withCollection(sinkOptions.getCollection())
                .withUsername(sinkOptions.getUsername())
                .withPassword(sinkOptions.getPassword())
                .withOptions(sinkOptions)
                .build();
    }

    /**
     *  if upstream kafka only has one topic with one partition, the changelog stream
     *  collected by sink is always ordered, it can flush directly.
     *
     * @param element
     * @param context
     */
    @Override
    public void write(RowData element, Context context) throws IOException, InterruptedException {
        flushRow(element);
    }

    /**
     * flush row data (changelog) by RowKind.
     *
     * @param rowData
     */
    private void flushRow(RowData rowData) throws IOException {
        BSONObject record = converter
                .toExternal(rowData, sinkOptions.getIgnoreNullField());

        // work around, retrieve kafka topic/partition from each changelog's
        // metadata column, check if there is more than one topic or partition.
        detectIfUsingMultiPartition(rowData);

        RetryUtil.retryWhenRuntimeException(() -> {
            switch (rowData.getRowKind()) {
                case INSERT:
                case UPDATE_AFTER:
                    provider.getCollection().upsert(
                            createMatcher(record),
                            createModifier(MODIFIER_SET, record),
                            null,
                            record, 0);
                    break;

                case UPDATE_BEFORE:
                case DELETE:
                    provider.getCollection().deleteRecords(createMatcher(record));
                    break;
            }
            return null;
        }, RetryUtil.DEFAULT_MAX_RETRY_TIMES, RetryUtil.DEFAULT_RETRY_DURATION, true);
    }

    /**
     * detect whether upstream kafka is using multi topics/partitions in retract mode
     *
     * - it's work around. for now, we only support consume changelog from one topic with one partition (kafka)
     * - it will throw exception to crash the job, when it detects there are more than one topic or one partition.
     *
     * @param rowData
     * @throws IOException
     */
    private void detectIfUsingMultiPartition(RowData rowData) throws IOException {
        String topic = readMetadata(rowData, ReadableMetadata.KAFKA_TOPIC);
        Integer partition = readMetadata(rowData, ReadableMetadata.KAFKA_PARTITION);

        if (topic == null || partition == null) {
            throw new SDBException(String.format(
                    "can't perform retract write without defining metadata columns: %s, %s.",
                    ReadableMetadata.KAFKA_TOPIC,
                    ReadableMetadata.KAFKA_PARTITION));
        }

        topicSet.add(topic);
        partitionSet.add(partition);

        if (topicSet.size() > 1 || partitionSet.size() > 1) {
            throw new IOException("sequoiadb connector does not support consume kafka multi topics or partitions.");
        }
    }

    // compute metadata position (in RowType)
    private int[] getMetadataPositions(RowType rowType) {
        return Stream.of(ReadableMetadata.values())
                .mapToInt(
                        m -> {
                            final int pos = rowType.getFieldNames().indexOf(m.key);
                            if (pos < 0) {
                                return -1;
                            }
                            return pos;
                        })
                .toArray();
    }

    // read metadata in physical row data (changelog)
    private <T> T readMetadata(RowData rowData, ReadableMetadata metadata) {
        final int pos = metadataPositions[metadata.ordinal()];
        if (pos < 0) {
            return null;
        }
        return (T) metadata.converter.convert(rowData, pos);
    }

    @Override
    public List<Void> prepareCommit(boolean flush) {
        return Lists.newArrayList();
    }

    @Override
    public List<Void> snapshotState(long checkpointId) throws IOException {
        return Lists.newArrayList();
    }


    private BSONObject createMatcher(BSONObject record) {
        BSONObject matcher = new BasicBSONObject();

        for (String upsertKey : upsertKeys) {
            matcher.put(upsertKey, record.get(upsertKey));
        }
        return matcher;
    }

    private BSONObject createModifier(String mType, BSONObject updater) {
        BSONObject modifier = new BasicBSONObject();
        modifier.put(mType, updater);
        return modifier;
    }

    @Override
    public void close() throws Exception {
        if (provider != null) {
            provider.close();
        }
    }

    enum ReadableMetadata {
        // Kafka Metadata
        KAFKA_TOPIC(
                "$kafka-topic",
                DataTypes.STRING().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getString(pos).toString();
                    }
                }),

        KAFKA_PARTITION(
                "$kafka-partition",
                DataTypes.INT().nullable(),
                new MetadataConverter() {
                    @Override
                    public Object convert(RowData consumedRow, int pos) {
                        return consumedRow.getInt(pos);
                    }
                }),
        ;

        final String key;

        final DataType dataType;

        final MetadataConverter converter;

        ReadableMetadata(String key, DataType dataType, MetadataConverter converter) {
            this.key = key;
            this.dataType = dataType;
            this.converter = converter;
        }
    }

    interface MetadataConverter {
        Object convert(RowData consumedRow, int pos);
    }

}

