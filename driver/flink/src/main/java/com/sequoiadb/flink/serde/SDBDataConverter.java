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

package com.sequoiadb.flink.serde;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.ByteUtil;
import org.apache.flink.table.data.*;
import org.apache.flink.table.data.binary.BinaryStringData;
import org.apache.flink.table.types.logical.DecimalType;
import org.apache.flink.table.types.logical.LogicalType;
import org.apache.flink.table.types.logical.LogicalTypeRoot;
import org.apache.flink.table.types.logical.RowType;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;

import java.io.Serializable;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class SDBDataConverter implements Serializable {

    private static final String AUTO_INDEX_ID_NAME = "_id";
    private static final String DATETIME_FORMAT_PATTERN = "yyyy-MM-dd.HH:mm:ss";

    private final SDBDeserializationConverter[] toInternalConverters;
    private final SDBSerializationConverter[] toExternalConverters;

    private final RowType rowType;
    private final List<RowType.RowField> rowFields;

    private static final List<String> METADATA_COLUMNS = new ArrayList<>();

    static {
        METADATA_COLUMNS.add("$kafka-topic");
        METADATA_COLUMNS.add("$kafka-partition");
        METADATA_COLUMNS.add("$extra-row-kind");
    }


    public SDBDataConverter(RowType rowType) {
        this.rowType = rowType;
        this.rowFields = rowType.getFields();

        this.toInternalConverters = new SDBDeserializationConverter[rowType.getFieldCount()];
        this.toExternalConverters = new SDBSerializationConverter[rowType.getFieldCount()];
        for (int i = 0; i < rowType.getFieldCount(); i++) {
            toInternalConverters[i] = createInternalConverter(rowType.getTypeAt(i));
            toExternalConverters[i] = createExternalConverter(rowFields.get(i).getType());
        }
    }

    public RowData toInternal(BSONObject record) {
        GenericRowData rowData = new GenericRowData(rowType.getFieldCount());

        for (int pos = 0; pos < rowData.getArity(); pos++) {
            String fieldName = rowFields.get(pos).getName();
            if (record.containsField(fieldName) && record.get(fieldName) != null) {
                try {
                    rowData.setField(pos, toInternalConverters[pos].deserialize(record.get(fieldName)));
                } catch (NumberFormatException ignored) { }
            }
        }
        return rowData;
    }

    public RowType getRowType() {
        return rowType;
    }

    /*
     * return a BSONObject built from rowdata
     * note: there is a limitation in BSON, that it can not exceed 16MB
     * consider here we only have strutured data with base datatypes
     * it will be hard to exceed it.
     *
     * @param rowData               incomming rowdata
     * @param ignoreNullField       ignore Null field flag
     * @return BSONObject
     */
    public BSONObject toExternal(RowData rowData, boolean ignoreNullField) {
        BSONObject record = new BasicBSONObject();
        for (int pos = 0; pos < rowData.getArity(); pos++) {
            // create field Getter to get value with correct type
            RowType.RowField rowField = rowFields.get(pos);
            RowData.FieldGetter fieldGetter = RowData.createFieldGetter(rowField.getType(), pos);

            // getting value from Rowdata
            String fieldName = rowField.getName();
            if (METADATA_COLUMNS.contains(fieldName)) { // skip metadata columns
                continue;
            }

            Object value = toExternalConverters[pos]
                    .serialize(fieldGetter.getFieldOrNull(rowData));
            
            // handle ignoreNullField option
            if (ignoreNullField && value == null) {
                continue;
            }
            
            // special case to handle OID, since ROWDATA don't have old type
            if (AUTO_INDEX_ID_NAME.equals(fieldName)) {
                value = new ObjectId(value.toString());
            }

            record.put(fieldName, value);
        }
        return record;
    }

    @FunctionalInterface
    interface SDBDeserializationConverter extends Serializable {
        Object deserialize(Object bsonField);
    }

    @FunctionalInterface
    interface SDBSerializationConverter extends Serializable {
        Object serialize(Object data);
    }

    // ================= BSON to RowData =================

    private static SDBDeserializationConverter createInternalConverter(LogicalType type) {
        LogicalTypeRoot typeRoot = type.getTypeRoot();
        switch (typeRoot) {
            case NULL:
                return val -> null;
            case TINYINT:
                return SDBDataConverter::toTinyInt;
            case SMALLINT:
                return SDBDataConverter::toSmallInt;
            case INTEGER:
                return SDBDataConverter::toInt;
            case BIGINT:
                return SDBDataConverter::toBigInt;
            case FLOAT:
                return SDBDataConverter::toFloat;
            case DOUBLE:
                return SDBDataConverter::toDouble;
            case DATE:
                return SDBDataConverter::toDate;
            case TIMESTAMP_WITHOUT_TIME_ZONE:
            case TIMESTAMP_WITH_LOCAL_TIME_ZONE:
            case TIMESTAMP_WITH_TIME_ZONE:
                return SDBDataConverter::toTimestamp;
            case BOOLEAN:
                return SDBDataConverter::toBoolean;
            case DECIMAL:
                return val -> toDecimal(type, val);
            case CHAR:
            case VARCHAR:
                return SDBDataConverter::toStringData;
            case BINARY:
            case VARBINARY:
                return SDBDataConverter::toBinary;

            default:
                throw new SDBException(String.format("unsupported type: %s.", typeRoot));
        }
    }

    private static byte toTinyInt(Object v) {
        if (v instanceof Integer)
            return ((Integer) v).byteValue();
        else if (v instanceof Long)
            return ((Long) v).byteValue();
        else if (v instanceof Double)
            return ((Double) v).byteValue();
        else if (v instanceof BigInteger)
            return ((BigInteger) v).byteValue();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).byteValue();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().byteValue();
        else if (v instanceof String)
            return Byte.parseByte((String) v);
        else if (v instanceof Boolean)
            return (byte) ((boolean) v ? 1 : 0);
        else if (v instanceof Date)
            return (byte) ((Date) v).getTime();
        else if (v instanceof BSONTimestamp)
            return (byte) (((BSONTimestamp) v).getTime() * 1000 + ((BSONTimestamp) v).getInc() / 1000);
        else
            return 0;
    }

    private static short toSmallInt(Object v) {
        if (v instanceof Integer)
            return ((Integer) v).shortValue();
        else if (v instanceof Long)
            return ((Long) v).shortValue();
        else if (v instanceof Double)
            return ((Double) v).shortValue();
        else if (v instanceof BigInteger)
            return ((BigInteger) v).shortValue();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).shortValue();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().shortValue();
        else if (v instanceof String)
            return Short.parseShort((String) v);
        else if (v instanceof Boolean)
            return (short) ((boolean) v ? 1 : 0);
        else if (v instanceof Date)
            return (short) ((Date) v).getTime();
        else if (v instanceof BSONTimestamp)
            return (short) (((BSONTimestamp) v).getTime() * 1000 + ((BSONTimestamp) v).getInc() / 1000);
        else
            return 0;
    }

    private static int toInt(Object v) {
        if (v instanceof Integer)
            return (int) v;
        else if (v instanceof Long)
            return ((Long) v).intValue();
        else if (v instanceof Double)
            return ((Double) v).intValue();
        else if (v instanceof BigInteger)
            return ((BigInteger) v).intValue();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).intValue();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().intValue();
        else if (v instanceof String)
            return Integer.parseInt((String) v);
        else if (v instanceof Boolean)
            return (boolean) v ? 1 : 0;
        else if (v instanceof Date)
            return (int) ((Date) v).getTime();
        else if (v instanceof BSONTimestamp)
            return ((BSONTimestamp) v).getTime() * 1000 + ((BSONTimestamp) v).getInc() / 1000;
        else return 0;
    }

    private static long toBigInt(Object v) {
        if (v instanceof Integer)
            return ((Integer) v).longValue();
        else if (v instanceof Long)
            return (long) v;
        else if (v instanceof Double)
            return ((Double) v).longValue();
        else if (v instanceof BigInteger)
            return ((BigInteger) v).longValue();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).longValue();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().longValue();
        else if (v instanceof String)
            return Long.parseLong((String) v);
        else if (v instanceof Boolean)
            return (boolean) v ? 1 : 0;
        else if (v instanceof Date)
            return ((Date) v).getTime();
        else if (v instanceof BSONTimestamp)
            return ((BSONTimestamp) v).getTime() * 1000L + ((BSONTimestamp) v).getInc() / 1000;
        else
            return 0;
    }

    private static float toFloat(Object v) {
        if (v instanceof Integer)
            return ((Integer) v).floatValue();
        else if (v instanceof Long)
            return ((Long) v).floatValue();
        else if (v instanceof Double)
            return ((Double) v).floatValue();
        else if (v instanceof BigInteger)
            return ((BigInteger) v).floatValue();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).floatValue();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().floatValue();
        else if (v instanceof String)
            return Float.parseFloat((String) v);
        else if (v instanceof Boolean)
            return (boolean) v ? 1.0f : 0.0f;
        else if (v instanceof Date)
            return (float) ((Date) v).getTime();
        else if (v instanceof BSONTimestamp)
            return (float) ((BSONTimestamp) v).getTime() * 1000 + ((BSONTimestamp) v).getInc() / 1000.0f;
        else
            return 0.0f;
    }

    private static double toDouble(Object v) {
        if (v instanceof Integer)
            return ((Integer) v).doubleValue();
        else if (v instanceof Long)
            return ((Long) v).doubleValue();
        else if (v instanceof Double)
            return (double) v;
        else if (v instanceof BigInteger)
            return ((BigInteger) v).doubleValue();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).doubleValue();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().doubleValue();
        else if (v instanceof String)
            return Double.parseDouble((String) v);
        else if (v instanceof Boolean)
            return (boolean) v ? 1.0 : 0.0;
        else if (v instanceof Date)
            return (double) ((Date) v).getTime();
        else if (v instanceof BSONTimestamp)
            return ((BSONTimestamp) v).getTime() * 1000L + ((BSONTimestamp) v).getInc() / 1000L;
        else
            return 0.0f;
    }

    private static int toDate(Object value) {
        int result = 0;

        if (value instanceof Integer) {
            result = (Integer) value;
        } else if (value instanceof Long) {
            result = ((Long) value).intValue();
        } else if (value instanceof Date) {
            Date date = (Date) value;
            result = (int) date.toInstant().atZone(ZoneId.systemDefault())
                    .toLocalDate().toEpochDay();
        } else if (value instanceof BSONTimestamp) {
            BSONTimestamp timestamp = (BSONTimestamp) value;
            result = (int) timestamp.toTimestamp().toLocalDateTime()
                    .toLocalDate().toEpochDay();
        } else if (value instanceof String) {
            DateTimeFormatter formatter = DateTimeFormatter.ofPattern(DATETIME_FORMAT_PATTERN);
            try {
                result = (int) LocalDate.parse(value.toString(), formatter)
                        .toEpochDay();
            } catch (DateTimeParseException ignored) {}
        }

        return result;
    }

    private static TimestampData toTimestamp(Object value) {
        TimestampData result = null;
        if (value instanceof Integer) {
            result = TimestampData.fromInstant(Instant.ofEpochMilli(((Integer) value).longValue()));
        } else if (value instanceof Long) {
            result = TimestampData.fromInstant(Instant.ofEpochMilli((long) value));
        } else if (value instanceof Date) {
            result = TimestampData.fromInstant(Instant.ofEpochMilli(((Date) value).getTime()));
        } else if (value instanceof BSONTimestamp) {
            result = TimestampData.fromInstant(((BSONTimestamp) value).toTimestamp().toInstant());
        } else if (value instanceof String) {
            DateTimeFormatter formatter = DateTimeFormatter.ofPattern(DATETIME_FORMAT_PATTERN);
            try {
                Instant instant = LocalDateTime.parse(value.toString(), formatter)
                        .atZone(ZoneId.systemDefault())
                        .toInstant();
                result = TimestampData.fromInstant(instant);
            } catch (DateTimeParseException ignored) {}
        }

        return result == null ?
                TimestampData.fromInstant(Instant.ofEpochMilli(0)) :
                result;
    }

    private static boolean toBoolean(Object v) {
        if (v instanceof Integer)
            return (int) v != 0;
        else if (v instanceof Long)
            return (long) v != 0;
        else if (v instanceof Double)
            return (double) v != 0;
        else if (v instanceof BigInteger)
            return !BigInteger.ZERO.equals(v);
        else if (v instanceof BigDecimal)
            return !BigDecimal.ZERO.equals(v);
        else if (v instanceof BSONDecimal)
            return !BigDecimal.ZERO.equals(((BSONDecimal) v).toBigDecimal());
        else if (v instanceof String)
            return "true".equalsIgnoreCase((String) v);
        else if (v instanceof Boolean)
            return (boolean) v;
        else return false;
    }

    private static DecimalData toDecimal(LogicalType type, Object v) {
        final int precision = ((DecimalType) type).getPrecision();
        final int scale = ((DecimalType) type).getScale();

        BigDecimal bd = null;
        if (v instanceof Integer)
            bd = BigDecimal.valueOf(((Integer) v).longValue());
        else if (v instanceof Long)
            bd = BigDecimal.valueOf((long) v);
        else if (v instanceof Double)
            bd = BigDecimal.valueOf((double) v);
        else if (v instanceof BigInteger)
            bd = new BigDecimal((BigInteger) v, 0);
        else if (v instanceof BigDecimal)
            bd = (BigDecimal) v;
        else if (v instanceof BSONDecimal)
            bd = ((BSONDecimal) v).toBigDecimal();
        else if (v instanceof String)
            bd = new BigDecimal((String) v);
        else if (v instanceof Boolean)
            bd = (boolean) v ? BigDecimal.ONE : BigDecimal.ZERO;
        else if (v instanceof Date)
            bd = BigDecimal.valueOf(((Date) v).getTime());
        else if (v instanceof BSONTimestamp)
            bd = BigDecimal.valueOf(((BSONTimestamp) v).getTime() * 1000L + ((BSONTimestamp) v).getInc() / 1000L);
        else
            bd = BigDecimal.ZERO;

        DecimalData dd = DecimalData.fromBigDecimal(bd, precision, scale);
        return dd == null ?
                DecimalData.fromBigDecimal(BigDecimal.ZERO, precision, scale) :
                dd;
    }

    private static StringData toStringData(Object v) {
        if (v instanceof BSONDecimal)
            return new BinaryStringData(((BSONDecimal) v).getValue());
        else if (v instanceof Date) {
            return new BinaryStringData(((Date) v).toInstant().atZone(ZoneId.systemDefault())
                    .toLocalDate().toString());
        }
        else if (v instanceof BSONTimestamp)
            return new BinaryStringData(((BSONTimestamp) v).toTimestamp().toString());
        else if (v instanceof Binary)
            return new BinaryStringData(new String(((Binary) v).getData(), StandardCharsets.UTF_8));
        else
            return new BinaryStringData(v.toString());
    }

    private static byte[] toBinary(Object v) {
        if (v instanceof Integer)
            return ByteUtil.toBytes((int) v);
        else if (v instanceof Long)
            return ByteUtil.toBytes((long) v);
        else if (v instanceof Double)
            return ByteUtil.toBytes((double) v);
        else if (v instanceof BigInteger)
            return ((BigInteger) v).toByteArray();
        else if (v instanceof BigDecimal)
            return ((BigDecimal) v).unscaledValue().toByteArray();
        else if (v instanceof BSONDecimal)
            return ((BSONDecimal) v).toBigDecimal().unscaledValue().toByteArray();
        else if (v instanceof String)
            return ((String) v).getBytes(StandardCharsets.UTF_8);
        else if (v instanceof ObjectId)
            return ((ObjectId) v).toByteArray();
        else if (v instanceof Boolean)
            return ByteUtil.toBytes((boolean) v);
        else if (v instanceof Date)
            return ByteUtil.toBytes(((Date) v).getTime());
        else if (v instanceof BSONTimestamp)
            return ByteUtil.toBytes(((BSONTimestamp) v).toTimestamp().getTime());
        else if (v instanceof Binary)
            return ((Binary) v).getData();
        else
            return new byte[]{};
    }

    // ================= BSON to RowData =================

    private static SDBSerializationConverter createExternalConverter(LogicalType type) {
        switch (type.getTypeRoot()) {
            case NULL:
            case TINYINT:
            case SMALLINT:
            case INTEGER:
            case BIGINT:
            case FLOAT:
            case DOUBLE:
            case BOOLEAN:
                return (v -> v);

            case DECIMAL:
                return (v -> v != null ? new BSONDecimal(((DecimalData) v).toBigDecimal()) : null);

            // using BSONDate
            case DATE:
                return (v -> v != null ?
                        BSONDate.valueOf(LocalDate.ofEpochDay(((Integer) v).longValue())) :
                        null);

            case TIMESTAMP_WITHOUT_TIME_ZONE:
            case TIMESTAMP_WITH_LOCAL_TIME_ZONE:
            case TIMESTAMP_WITH_TIME_ZONE:
                return (v -> {
                    TimestampData ts = (TimestampData) v;
                    BSONTimestamp result = null;
                    if (ts != null) {
                        Instant instant = ts.toInstant();
                        result = new BSONTimestamp((int) instant.getEpochSecond(), instant.getNano() / 1000);
                    }
                    return result;
                });

            case CHAR:
            case VARCHAR:
                return (v -> v != null ? v.toString() : null);

            case BINARY:
            case VARBINARY:
                return (v -> v != null ? new Binary((byte[]) v) : null);

            default:
                throw new SDBException(String.format("unsupported type: %s.", type.getTypeRoot()));
        }
    }

}
