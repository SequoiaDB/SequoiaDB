/*
 * Copyright 2017 SequoiaDB Inc.
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

package com.sequoiadb.spark;

import org.apache.spark.sql.Row;
import org.apache.spark.sql.Row$;
import org.apache.spark.sql.types.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;
import org.junit.Test;
import scala.collection.JavaConversions;
import scala.collection.mutable.ArrayBuffer;
import scala.collection.mutable.ArrayBuffer$;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.sql.Timestamp;
import java.util.Arrays;
import java.util.Date;
import java.util.UUID;
import java.util.regex.Pattern;

import static org.junit.Assert.assertEquals;

public class TestBSONConverter {
    private static final BooleanType$ SparkBooleanType = BooleanType$.MODULE$;
    private static final ByteType$ SparkByteType = ByteType$.MODULE$;
    private static final ShortType$ SparkShortType = ShortType$.MODULE$;
    private static final IntegerType$ SparkIntType = IntegerType$.MODULE$;
    private static final LongType$ SparkLongType = LongType$.MODULE$;
    private static final FloatType$ SparkFloatType = FloatType$.MODULE$;
    private static final DoubleType$ SparkDoubleType = DoubleType$.MODULE$;
    private static final DecimalType$ SparkDecimalType = DecimalType$.MODULE$;
    private static final DateType$ SparkDateType = DateType$.MODULE$;
    private static final TimestampType$ SparkTimestampType = TimestampType$.MODULE$;
    private static final StringType$ SparkStringType = StringType$.MODULE$;
    private static final BinaryType$ SparkBinaryType = BinaryType$.MODULE$;
    private static final NullType$ SparkNullType = NullType$.MODULE$;

    private final boolean trueVal = true;
    private final boolean falseVal = false;
    private final float floatVal = 1.1f;
    private final double doubleVal = 1.2f;
    private final byte byteVal = 127;
    private final short shortVal = 20000;
    private final int intVal = 1234567;
    private final long longVal = 123456789012L;
    private final BigInteger bigIntVal =
        new BigInteger("9223372036854775807");
    private final BigDecimal bigDecimalVal =
        new BigDecimal("1234567890123456789012345678901234567890.123456789");
    private final BSONDecimal bsonDecimalVal =
        new BSONDecimal("1234567890123456789012345678901234567890.123456789");
    private final BSONTimestamp bsonTimestampVal =
        new BSONTimestamp((int) (new Date().getTime() / 1000), 123456);
    private final Date dateVal = new Date();
    private final String strVal = "hello";
    private final Binary binaryVal = new Binary("hello world!".getBytes());
    private final UUID uuidVal = UUID.randomUUID();
    private final ObjectId oidVal = new ObjectId();
    private final Pattern regexVal = Pattern.compile("$123");
    private final MaxKey maxKeyVal = new MaxKey();
    private final MinKey minKeyVal = new MinKey();
    private final Code codeVal = new Code("hello");
    private final CodeWScope codeWScopeVal = new CodeWScope("hello", new BasicBSONObject());
    private final Symbol symbolVal = new Symbol("symbol");

    @Test
    public void testSingleBsonValueType() {
        assertEquals(SparkBooleanType, BSONConverter.typeOfData(trueVal));
        assertEquals(SparkBooleanType, BSONConverter.typeOfData(falseVal));
        assertEquals(SparkFloatType, BSONConverter.typeOfData(floatVal));
        assertEquals(SparkDoubleType, BSONConverter.typeOfData(doubleVal));
        assertEquals(SparkByteType, BSONConverter.typeOfData(byteVal));
        assertEquals(SparkShortType, BSONConverter.typeOfData(shortVal));
        assertEquals(SparkIntType, BSONConverter.typeOfData(intVal));
        assertEquals(SparkLongType, BSONConverter.typeOfData(longVal));
        assertEquals(new DecimalType(38, 0), BSONConverter.typeOfData(bigIntVal));
        assertEquals(new DecimalType(38, 9), BSONConverter.typeOfData(bigDecimalVal));
        assertEquals(new DecimalType(38, 9), BSONConverter.typeOfData(bsonDecimalVal));
        assertEquals(SparkTimestampType, BSONConverter.typeOfData(bsonTimestampVal));
        assertEquals(SparkDateType, BSONConverter.typeOfData(dateVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(strVal));
        assertEquals(SparkBinaryType, BSONConverter.typeOfData(binaryVal));
        assertEquals(SparkBinaryType, BSONConverter.typeOfData(uuidVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(oidVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(regexVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(maxKeyVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(minKeyVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(codeVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(codeWScopeVal));
        assertEquals(SparkStringType, BSONConverter.typeOfData(symbolVal));
        assertEquals(SparkNullType, BSONConverter.typeOfData(null));
    }

    @Test
    public void testTypeOfArray() {
        BasicBSONList array = new BasicBSONList();

        array.put(0, falseVal);
        array.put(1, trueVal);
        assertEquals(new ArrayType(SparkBooleanType, false),
            BSONConverter.typeOfArray(JavaConversions.asScalaBuffer(array)));
        array.clear();

        array.put(0, floatVal);
        array.put(1, null);
        assertEquals(new ArrayType(SparkFloatType, true),
            BSONConverter.typeOfArray(JavaConversions.asScalaBuffer(array)));
        array.clear();

        array.put(0, bsonDecimalVal);
        array.put(1, null);
        assertEquals(new ArrayType(new DecimalType(38, 9), true),
            BSONConverter.typeOfArray(JavaConversions.asScalaBuffer(array)));
        array.clear();

        array.put(0, uuidVal);
        array.put(1, null);
        assertEquals(new ArrayType(SparkBinaryType, true),
            BSONConverter.typeOfArray(JavaConversions.asScalaBuffer(array)));
        array.clear();
    }

    @Test
    public void testBsonType() {
        BSONObject obj = new BasicBSONObject();
        obj.put("bool", trueVal);
        obj.put("byte", byteVal);
        obj.put("short", shortVal);
        obj.put("int", intVal);
        obj.put("long", longVal);
        obj.put("float", floatVal);
        obj.put("double", doubleVal);
        obj.put("decimal", bigDecimalVal);
        obj.put("null", null);
        obj.put("string", strVal);
        obj.put("date", dateVal);
        obj.put("timestamp", bsonTimestampVal);
        obj.put("binary", binaryVal);

        StructField f1 = new StructField("bool", SparkBooleanType, true, Metadata.empty());
        StructField f2 = new StructField("byte", SparkByteType, true, Metadata.empty());
        StructField f3 = new StructField("short", SparkShortType, true, Metadata.empty());
        StructField f4 = new StructField("int", SparkIntType, true, Metadata.empty());
        StructField f5 = new StructField("long", SparkLongType, true, Metadata.empty());
        StructField f6 = new StructField("float", SparkFloatType, true, Metadata.empty());
        StructField f7 = new StructField("double", SparkDoubleType, true, Metadata.empty());
        StructField f8 = new StructField("decimal", new DecimalType(38, 9), true, Metadata.empty());
        StructField f9 = new StructField("null", SparkNullType, true, Metadata.empty());
        StructField f10 = new StructField("string", SparkStringType, true, Metadata.empty());
        StructField f11 = new StructField("date", SparkDateType, true, Metadata.empty());
        StructField f12 = new StructField("timestamp", SparkTimestampType, true, Metadata.empty());
        StructField f13 = new StructField("binary", SparkBinaryType, true, Metadata.empty());

        StructType type = new StructType(new StructField[]{
            f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13
        });

        assertEquals(type, BSONConverter.typeOfData(obj));
    }

    @Test
    public void testByteCompatibleType() {
        assertEquals(SparkByteType, BSONConverter.compatibleType(SparkByteType, SparkByteType));
        assertEquals(SparkShortType, BSONConverter.compatibleType(SparkByteType, SparkShortType));
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkByteType, SparkIntType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkByteType, SparkLongType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkByteType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkByteType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkByteType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkByteType, BSONConverter.compatibleType(SparkByteType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkByteType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkByteType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkByteType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkByteType, SparkBinaryType));
    }

    @Test
    public void testShortCompatibleType() {
        assertEquals(SparkShortType, BSONConverter.compatibleType(SparkShortType, SparkByteType));
        assertEquals(SparkShortType, BSONConverter.compatibleType(SparkShortType, SparkShortType));
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkShortType, SparkIntType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkShortType, SparkLongType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkShortType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkShortType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkShortType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkShortType, BSONConverter.compatibleType(SparkShortType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkShortType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkShortType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkShortType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkShortType, SparkBinaryType));
    }

    @Test
    public void testIntCompatibleType() {
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkIntType, SparkByteType));
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkIntType, SparkShortType));
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkIntType, SparkIntType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkIntType, SparkLongType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkIntType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkIntType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkIntType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkIntType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkIntType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkIntType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkIntType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkIntType, SparkBinaryType));
    }

    @Test
    public void testLongCompatibleType() {
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkLongType, SparkByteType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkLongType, SparkShortType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkLongType, SparkIntType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkLongType, SparkLongType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkLongType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkLongType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkLongType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkLongType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkLongType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkLongType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkLongType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkLongType, SparkBinaryType));
    }

    @Test
    public void testFloatCompatibleType() {
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkFloatType, SparkByteType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkFloatType, SparkShortType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkFloatType, SparkIntType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkFloatType, SparkLongType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkFloatType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkFloatType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkFloatType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkFloatType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkFloatType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkFloatType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkFloatType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkFloatType, SparkBinaryType));
    }

    @Test
    public void testDoubleCompatibleType() {
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkByteType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkShortType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkIntType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkLongType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkDoubleType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkDoubleType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDoubleType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDoubleType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDoubleType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDoubleType, SparkBinaryType));
    }

    @Test
    public void testDecimalCompatibleType() {
        DecimalType sysDefaultDecimal = SparkDecimalType.SYSTEM_DEFAULT();
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkByteType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkShortType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkIntType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkLongType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkFloatType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkDoubleType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, sysDefaultDecimal));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(sysDefaultDecimal, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(sysDefaultDecimal, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(sysDefaultDecimal, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(sysDefaultDecimal, SparkBinaryType));
        assertEquals(sysDefaultDecimal, BSONConverter.compatibleType(sysDefaultDecimal, SparkDecimalType.BigIntDecimal()));
        assertEquals(new DecimalType(38, 15),
            BSONConverter.compatibleType(SparkDecimalType.BigIntDecimal(), SparkDecimalType.DoubleDecimal()));
    }

    @Test
    public void testNullCompatibleType() {
        assertEquals(SparkByteType, BSONConverter.compatibleType(SparkNullType, SparkByteType));
        assertEquals(SparkShortType, BSONConverter.compatibleType(SparkNullType, SparkShortType));
        assertEquals(SparkIntType, BSONConverter.compatibleType(SparkNullType, SparkIntType));
        assertEquals(SparkLongType, BSONConverter.compatibleType(SparkNullType, SparkLongType));
        assertEquals(SparkFloatType, BSONConverter.compatibleType(SparkNullType, SparkFloatType));
        assertEquals(SparkDoubleType, BSONConverter.compatibleType(SparkNullType, SparkDoubleType));
        assertEquals(SparkDecimalType.SYSTEM_DEFAULT(),
            BSONConverter.compatibleType(SparkNullType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkNullType, BSONConverter.compatibleType(SparkNullType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkNullType, SparkStringType));
        assertEquals(SparkDateType, BSONConverter.compatibleType(SparkNullType, SparkDateType));
        assertEquals(SparkTimestampType, BSONConverter.compatibleType(SparkNullType, SparkTimestampType));
        assertEquals(SparkBinaryType, BSONConverter.compatibleType(SparkNullType, SparkBinaryType));
    }

    @Test
    public void testDateCompatibleType() {
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkByteType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkShortType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkIntType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkLongType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkFloatType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkDoubleType));
        assertEquals(SparkStringType,
            BSONConverter.compatibleType(SparkDateType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkDateType, BSONConverter.compatibleType(SparkDateType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkStringType));
        assertEquals(SparkDateType, BSONConverter.compatibleType(SparkDateType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkDateType, SparkBinaryType));
    }

    @Test
    public void testTimestampCompatibleType() {
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkByteType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkShortType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkIntType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkLongType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkFloatType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkDoubleType));
        assertEquals(SparkStringType,
            BSONConverter.compatibleType(SparkTimestampType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkTimestampType, BSONConverter.compatibleType(SparkTimestampType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkDateType));
        assertEquals(SparkTimestampType, BSONConverter.compatibleType(SparkTimestampType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkTimestampType, SparkBinaryType));
    }

    @Test
    public void testStringCompatibleType() {
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkByteType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkShortType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkIntType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkLongType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkFloatType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkDoubleType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkTimestampType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkStringType, SparkBinaryType));
    }

    @Test
    public void testBinaryCompatibleType() {
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkByteType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkShortType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkIntType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkLongType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkFloatType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkDoubleType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkDecimalType.SYSTEM_DEFAULT()));
        assertEquals(SparkBinaryType, BSONConverter.compatibleType(SparkBinaryType, SparkNullType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkStringType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkDateType));
        assertEquals(SparkStringType, BSONConverter.compatibleType(SparkBinaryType, SparkTimestampType));
        assertEquals(SparkBinaryType, BSONConverter.compatibleType(SparkBinaryType, SparkBinaryType));
    }

    @Test
    public void testSingleBsonRowConversion() {
        BSONObject obj = new BasicBSONObject();
        obj.put("bool", trueVal);
        obj.put("byte", byteVal);
        obj.put("short", shortVal);
        obj.put("int", intVal);
        obj.put("long", longVal);
        obj.put("float", floatVal);
        obj.put("double", doubleVal);
        obj.put("bigInt", bigIntVal);
        obj.put("bigDecimal", bigDecimalVal);
        obj.put("bsonDecimal", bsonDecimalVal);
        obj.put("null", null);
        obj.put("string", strVal);
        obj.put("date", dateVal);
        obj.put("timestamp", bsonTimestampVal);
        obj.put("binary", binaryVal);

        Timestamp ts = new Timestamp(((long) bsonTimestampVal.getTime() * 1000));
        ts.setNanos(bsonTimestampVal.getInc() * 1000);

        Row expectedRow = Row$.MODULE$.apply(JavaConversions.asScalaBuffer(Arrays.asList(
            new Object[]{
                trueVal, byteVal, shortVal, intVal, longVal, floatVal, doubleVal,
                Decimal.apply(bigIntVal),
                Decimal.apply(bigDecimalVal),
                Decimal.apply(bsonDecimalVal.toBigDecimal()),
                null,
                strVal,
                new java.sql.Date(dateVal.getTime()),
                ts,
                binaryVal.getData()
            }
        )));

        StructType schema = (StructType) BSONConverter.typeOfData(obj);

        Row row = BSONConverter.bsonToRow(obj, schema);

        assertEquals(expectedRow, row);

        BSONObject expectedObj = new BasicBSONObject();
        expectedObj.put("bool", trueVal);
        expectedObj.put("byte", (int)byteVal);
        expectedObj.put("short", (int)shortVal);
        expectedObj.put("int", intVal);
        expectedObj.put("long", longVal);
        expectedObj.put("float", floatVal);
        expectedObj.put("double", doubleVal);
        expectedObj.put("bigInt", new BSONDecimal(bigIntVal.toString()));
        expectedObj.put("bigDecimal", new BSONDecimal(bigDecimalVal));
        expectedObj.put("bsonDecimal", bsonDecimalVal);
        expectedObj.put("null", null);
        expectedObj.put("string", strVal);
        expectedObj.put("date", dateVal);
        expectedObj.put("timestamp", bsonTimestampVal);
        expectedObj.put("binary", binaryVal);

        BSONObject obj2 = BSONConverter.rowToBson(row, schema);

        assertEquals(expectedObj, obj2);
    }

    @Test
    public void testBsonArrayConversion() {
        BasicBSONList array = new BasicBSONList();
        array.put("0", byteVal);
        array.put("1", shortVal);
        array.put("2", intVal);
        array.put("3", longVal);

        BSONObject obj = new BasicBSONObject();
        obj.put("array", array);

        StructType schema = (StructType) BSONConverter.typeOfData(obj);

        ArrayBuffer arrayBuffer = ArrayBuffer$.MODULE$.apply(
            JavaConversions.asScalaBuffer(Arrays.asList(
                new Object[]{
                    byteVal, shortVal, intVal, longVal
                }
            ))
        );

        Row expectedRow = Row$.MODULE$.apply(
            JavaConversions.asScalaBuffer(Arrays.asList(
                new Object[]{arrayBuffer}
            ))
        );

        Row row = BSONConverter.bsonToRow(obj, schema);

        assertEquals(expectedRow, row);

        BasicBSONList array2 = new BasicBSONList();
        array2.put("0", (long) byteVal);
        array2.put("1", (long) shortVal);
        array2.put("2", (long) intVal);
        array2.put("3", longVal);

        BSONObject expectedObj = new BasicBSONObject();
        expectedObj.put("array", array2);

        BSONObject obj2 = BSONConverter.rowToBson(row, schema);

        assertEquals(expectedObj, obj2);
    }
}
