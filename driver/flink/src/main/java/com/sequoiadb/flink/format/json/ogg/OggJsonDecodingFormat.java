/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.flink.format.json.ogg;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.metadata.ExtraRowKind;
import org.apache.flink.api.common.serialization.DeserializationSchema;
import org.apache.flink.api.common.typeinfo.TypeInformation;
import org.apache.flink.formats.common.TimestampFormat;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.connector.ChangelogMode;
import org.apache.flink.table.connector.format.DecodingFormat;
import org.apache.flink.table.connector.source.DynamicTableSource;
import org.apache.flink.table.data.GenericRowData;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.data.TimestampData;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.utils.DataTypeUtils;
import org.apache.flink.types.RowKind;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeFormatterBuilder;
import java.time.temporal.TemporalAccessor;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static com.sequoiadb.flink.format.json.ogg.OggJsonDeserializationSchema.MetadataConverter;
import static java.time.temporal.ChronoField.*;

/** {@link DecodingFormat} for Ogg using JSON encoding. */
public class OggJsonDecodingFormat implements DecodingFormat<DeserializationSchema<RowData>> {

    private static final DateTimeFormatter DEFAULT_TIMESTAMP_FORMATTER =
            new DateTimeFormatterBuilder()
                    .appendPattern("yyyy-[MM][M]-[dd][d]")
                    .optionalStart()
                    .appendPattern(" [HH][H]:[mm][m]:[ss][s]")
                    .appendFraction(NANO_OF_SECOND, 0, 9, true)
                    .optionalEnd()
                    .toFormatter();

    // --------------------------------------------------------------------------------------------
    // Mutable attributes
    // --------------------------------------------------------------------------------------------

    private List<String> metadataKeys;

    // --------------------------------------------------------------------------------------------
    // Ogg-specific attributes
    // --------------------------------------------------------------------------------------------

    private final boolean ignoreParseErrors;
    private final TimestampFormat timestampFormat;
    private final String[] upsertKeys;
    private final String cPartitionPolicy;

    public OggJsonDecodingFormat(
            boolean ignoreParseErrors, TimestampFormat timestampFormat, String[] upsertKey, String cPartitionPolicy) {
        this.ignoreParseErrors = ignoreParseErrors;
        this.timestampFormat = timestampFormat;
        this.upsertKeys = upsertKey;
        this.metadataKeys = Collections.emptyList();
        this.cPartitionPolicy = cPartitionPolicy;
    }

    @Override
    public DeserializationSchema<RowData> createRuntimeDecoder(
            DynamicTableSource.Context context, DataType physicalDataType) {

        final List<ReadableMetadata> readableMetadata =
                metadataKeys.stream()
                        .map(
                                k ->
                                        Stream.of(ReadableMetadata.values())
                                                .filter(rm -> rm.key.equals(k))
                                                .findFirst()
                                                .orElseThrow(IllegalStateException::new))
                        .collect(Collectors.toList());

        final List<DataTypes.Field> metadataFields =
                readableMetadata.stream()
                        .map(m -> DataTypes.FIELD(m.key, m.dataType))
                        .collect(Collectors.toList());

        final DataType producedDataType =
                DataTypeUtils.appendRowFields(physicalDataType, metadataFields);

        final TypeInformation<RowData> producedTypeInfo =
                context.createTypeInformation(producedDataType);

        return new OggJsonDeserializationSchema(
                physicalDataType,
                readableMetadata,
                producedTypeInfo,
                ignoreParseErrors,
                timestampFormat,
                upsertKeys,
                cPartitionPolicy);
    }

    // --------------------------------------------------------------------------------------------
    // Metadata handling
    // --------------------------------------------------------------------------------------------

    @Override
    public Map<String, DataType> listReadableMetadata() {
        final Map<String, DataType> metadataMap = new LinkedHashMap<>();
        Stream.of(ReadableMetadata.values())
                .forEachOrdered(m -> metadataMap.put(m.key, m.dataType));
        return metadataMap;
    }

    @Override
    public void applyReadableMetadata(List<String> metadataKeys) {
        this.metadataKeys = metadataKeys;
    }

    @Override
    public ChangelogMode getChangelogMode() {
        return ChangelogMode.newBuilder()
                .addContainedKind(RowKind.INSERT)
                .addContainedKind(RowKind.UPDATE_BEFORE)
                .addContainedKind(RowKind.UPDATE_AFTER)
                .addContainedKind(RowKind.DELETE)
                .build();
    }

    private static LocalDateTime fromTemporalAccessor(TemporalAccessor accessor, int precision) {
        // complement year with 1970
        int year = accessor.isSupported(YEAR) ? accessor.get(YEAR) : 1970;
        // complement month of year with 1
        int month = accessor.isSupported(MONTH_OF_YEAR) ?
                accessor.get(MONTH_OF_YEAR) : 1;
        // complement day of month with 1
        int day = accessor.isSupported(DAY_OF_MONTH) ? accessor.get(DAY_OF_MONTH) : 1;

        // complement hour of day with 0
        int hour = accessor.isSupported(HOUR_OF_DAY) ? accessor.get(HOUR_OF_DAY) : 0;
        // complement minute of hour with 0
        int minute = accessor.isSupported(MINUTE_OF_HOUR) ?
                accessor.get(MINUTE_OF_HOUR) : 0;
        // complement second of minute with 0
        int second = accessor.isSupported(SECOND_OF_MINUTE) ? accessor.get(SECOND_OF_MINUTE) : 0;
        // complement nano of second with 0
        int nanoOfSecond = accessor.isSupported(NANO_OF_SECOND) ?
                accessor.get(NANO_OF_SECOND) : 0;

        if (precision == 0) {
            nanoOfSecond = 0;
        } else if (precision != 9) {
            nanoOfSecond = (int) floor(nanoOfSecond, powerX(10, 9 - precision));
        }

        return LocalDateTime.of(year, month, day, hour, minute, second, nanoOfSecond);
    }

    private static long floor(long a, long b) {
        long r = a % b;
        if (r < 0) {
            return a - r - b;
        } else {
            return a - r;
        }
    }

    private static long powerX(long a, long b) {
        long x = 1;
        while (b > 0) {
            x *= a;
            --b;
        }
        return x;
    }

    /** List of metadata that can be read with this format. */
    enum ReadableMetadata {
        TABLE(
                "table",
                DataTypes.STRING().nullable(),
                DataTypes.FIELD("table", DataTypes.STRING()),
                new MetadataConverter() {
                    private static final long serialVersionUID = 1L;

                    @Override
                    public Object convert(GenericRowData row, int pos) {
                        return row.getString(pos);
                    }
                }),

        PRIMARY_KEYS(
                "primary-keys",
                DataTypes.ARRAY(DataTypes.STRING()).nullable(),
                DataTypes.FIELD("primary_keys", DataTypes.ARRAY(DataTypes.STRING())),
                new MetadataConverter() {
                    private static final long serialVersionUID = 1L;

                    @Override
                    public Object convert(GenericRowData row, int pos) {
                        return row.getArray(pos);
                    }
                }),

        INGESTION_TIMESTAMP(
                "ingestion-timestamp",
                DataTypes.TIMESTAMP_WITH_LOCAL_TIME_ZONE(6).nullable(),
                DataTypes.FIELD("current_ts", DataTypes.STRING()),
                new MetadataConverter() {
                    private static final long serialVersionUID = 1L;

                    @Override
                    public Object convert(GenericRowData row, int pos) {
                        if (row.isNullAt(pos)) {
                            return null;
                        }
                        String dateStr = row.getString(pos).toString();
                        LocalDateTime ldt = fromTemporalAccessor(
                                DEFAULT_TIMESTAMP_FORMATTER.parse(dateStr), 6);
                        return TimestampData.fromLocalDateTime(ldt);
                    }
                }),

        EVENT_TIMESTAMP(
                "event-timestamp",
                DataTypes.TIMESTAMP_WITH_LOCAL_TIME_ZONE(6).nullable(),
                DataTypes.FIELD("op_ts", DataTypes.STRING()),
                new MetadataConverter() {
                    private static final long serialVersionUID = 1L;

                    @Override
                    public Object convert(GenericRowData row, int pos) {
                        if (row.isNullAt(pos)) {
                            return null;
                        }
                        String dateStr = row.getString(pos).toString();
                        LocalDateTime ldt = fromTemporalAccessor(
                                DEFAULT_TIMESTAMP_FORMATTER.parse(dateStr), 6);
                        return TimestampData.fromLocalDateTime(ldt);
                    }
                }),

        EXTRA_ROW_KIND(
                "$extra-row-kind",
                DataTypes.INT().nullable(),
                DataTypes.FIELD("op_type", DataTypes.INT()),
                new MetadataConverter() {
                    private static final long serialVersionUID = 1;

                    @Override
                    public Object convert(GenericRowData row, int pos) {
                        String opType = row.getString(pos).toString();
                        switch (opType) {
                            case "I":
                                return ExtraRowKind.INSERT.getCode();
                            case "U":
                                return ExtraRowKind.UPDATE_AFT.getCode();
                            case "D":
                                return ExtraRowKind.DELETE.getCode();

                            default:
                                throw new SDBException(String.format(
                                        "unsupported op type %s for retract-ogg",
                                        opType));
                        }
                    }
                }),
        ;

        final String key;

        final DataType dataType;

        final DataTypes.Field requiredJsonField;

        final MetadataConverter converter;

        ReadableMetadata(
                String key,
                DataType dataType,
                DataTypes.Field requiredJsonField,
                MetadataConverter converter) {
            this.key = key;
            this.dataType = dataType;
            this.requiredJsonField = requiredJsonField;
            this.converter = converter;
        }
    }

}
