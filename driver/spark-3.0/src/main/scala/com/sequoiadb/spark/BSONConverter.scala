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

package com.sequoiadb.spark

import org.apache.spark.sql.Row
import org.apache.spark.sql.catalyst.analysis.{DecimalPrecision, TypeCoercion}
import org.apache.spark.sql.catalyst.expressions.GenericRow
import org.apache.spark.sql.types._
import org.bson.types._
import org.bson.{BSONObject, BasicBSONObject}

import java.lang.reflect.Field
import java.math.BigInteger
import java.sql.{Date, Timestamp}
import java.text.SimpleDateFormat
import java.time.format.DateTimeFormatter
import java.time.{Instant, LocalDate, LocalDateTime, ZoneId}
import java.util.UUID
import scala.collection.JavaConversions._
import scala.collection.mutable
import scala.collection.mutable.ArrayBuffer
import scala.math.min

private[spark] object BSONConverter {

    val EPOCH_DATE: LocalDate = LocalDate.of(1970, 1, 1)
    val DATETIME_FORMAT_PATTERN: String = "yyyy-MM-dd.HH:mm:ss"

    /**
      * Convert Row to BSONObject by schema
      *
      * @param row    the source data in Row
      * @param schema the schema for BSONObject
      * @return BSONObject
      */
    def rowToBson(row: Row, schema: StructType): BSONObject = {
        val attMap: Map[String, Any] = schema.fields.zipWithIndex.map {
            case (att, idx) => (att.name, toBsonObj(row(idx), att.dataType))
        }.toMap
        val obj: BSONObject = new BasicBSONObject()
        obj.putAll(attMap)
        obj
    }

    // convert single Row field to BSONObject field
    private def toBsonObj(value: Any, dataType: DataType): Any = {
        Option(value).map { v =>
            (dataType, v) match {
                case (ArrayType(elementType, _), array: Seq[Any@unchecked]) =>
                    val bsonList: BasicBSONList = new BasicBSONList
                    array.zipWithIndex.foreach {
                        case (obj, i) => bsonList.put(i, toBsonObj(obj, elementType))
                    }
                    bsonList
                case (struct: StructType, value: GenericRow) =>
                    rowToBson(value, struct)
                case (mapType: MapType, map: Map[String@unchecked, Any@unchecked]) =>
                    if (mapType.keyType == StringType) {
                        val bson: BSONObject = new BasicBSONObject()
                        for ((key, value) <- map) {
                            val v = toBsonObj(value, mapType.valueType)
                            bson.put(key, v)
                        }
                        bson
                    } else {
                        null
                    }
                case (_: DecimalType, value: java.math.BigDecimal) =>
                    new BSONDecimal(value)
                case (_: DecimalType, value: Decimal) =>
                    new BSONDecimal(value.toJavaBigDecimal)
                case (_: TimestampType, value: Timestamp) =>
                    new BSONTimestamp((value.getTime / 1000).toInt, value.getNanos / 1000)
                case (_: TimestampType, value: Instant) =>
                    new BSONTimestamp(value.getEpochSecond.toInt, value.getNano / 1000)
                case (_: ByteType, value: Byte) => value.toInt
                case (_: ShortType, value: Short) => value.toInt
                case (_: BinaryType, value: Array[Byte]) => new Binary(value)
                case (_: CalendarIntervalType, _) => null
                case (_: DateType, value: java.time.LocalDate) => BSONDate.valueOf(value)
                case _ => v
            }
        }.orNull
    }

    /**
      * Convert BSONObject to Row by schema
      *
      * @param obj    the source BSONObject
      * @param schema the schema of Row
      * @return Row
      */
    def bsonToRow(obj: BSONObject, schema: StructType, java8Enabled: Boolean): Row = {
        val values: Seq[Any] = schema.fields.map {
            case StructField(name, dataType, _, _) =>
                Option(obj.get(name)).map(toRowField(_, dataType, java8Enabled)).orNull
        }
        Row.fromSeq(values)
    }

    /* consider BSONObject element types
    value match {
            case value: java.lang.Boolean
            case value: java.lang.Float
            case value: java.lang.Double
            case value: java.lang.Byte
            case value: java.lang.Short
            case value: java.lang.Integer
            case value: java.lang.Long
            case value: java.math.BigInteger
            case value: java.math.BigDecimal
            case value: BSONDecimal
            case value: BSONTimestamp
            case value: java.util.Date
            case value: String
            case value: Binary
            case value: UUID
            case value: ObjectId
            case value: BasicBSONList
            case value: BSONObject
            case value: Pattern
            case value: MaxKey
            case value: MinKey
            case value: Code
            case value: CodeWScope
            case value: Symbol
        }
    */

    // convert BSONObject field to Row field
    private def toRowField(value: Any, desiredType: DataType, java8APIEnabled: Boolean): Any = {
        try {
            desiredType match {
                case obj: StructType =>
                    bsonToRow(value.asInstanceOf[BSONObject], obj, java8APIEnabled)
                case ArrayType(elementType, _) =>
                    value.asInstanceOf[BasicBSONList].map(toRowField(_, elementType, java8APIEnabled))
                case BinaryType => toBinary(value)
                case BooleanType => toBoolean(value)
                case ByteType => toByte(value)
                //case CalendarIntervalType => toCalendarInterval(value)
                case DateType =>
                    if (java8APIEnabled) toLocalDate(value) else toDate(value)
                case DecimalType() => toDecimal(value)
                case DoubleType => toDouble(value)
                case FloatType => toFloat(value)
                case IntegerType => toInt(value)
                case LongType => toLong(value)
                case MapType(StringType, valueType, _) =>
                    val obj = value.asInstanceOf[BSONObject]
                    val map = new mutable.HashMap[String, Any]
                    obj.keySet().foreach { key =>
                        val value = obj.get(key)
                        map += (key -> toRowField(value, valueType, java8APIEnabled))
                    }
                    map
                case NullType => null
                case ShortType => toShort(value)
                case StringType => toString(value)
                case TimestampType =>
                    if (java8APIEnabled) toInstant(value) else toTimestamp(value)
                case _ =>
                    throw new SdbException(s"Unsupported data type conversion [${value.getClass}}, $desiredType]")
            }
        } catch {
            case sdbEx: SdbException => throw sdbEx
            case _: Exception => null
        }
    }

    private def toBinary(value: Any): Array[Byte] = {
        value match {
            case value: java.lang.Boolean => ByteUtil.getBytes(value)
            case value: java.lang.Float => ByteUtil.getBytes(value)
            case value: java.lang.Double => ByteUtil.getBytes(value)
            case value: java.lang.Byte => Array[Byte](value)
            case value: java.lang.Short => ByteUtil.getBytes(value)
            case value: java.lang.Integer => ByteUtil.getBytes(value)
            case value: java.lang.Long => ByteUtil.getBytes(value)
            case value: java.math.BigInteger => value.toByteArray
            case value: java.math.BigDecimal => ByteUtil.getBytes(value.doubleValue())
            case value: BSONDecimal => ByteUtil.getBytes(value.toBigDecimal.doubleValue())
            case value: BSONTimestamp => ByteUtil.getBytes(value.getTime.toLong * 1000 + value.getInc / 1000)
            case value: java.util.Date => ByteUtil.getBytes(value.getTime)
            case value: String => ByteUtil.getBytes(value)
            case value: Binary => value.getData
            case value: UUID =>
                val buffer = ArrayBuffer[Byte]()
                buffer ++= ByteUtil.getBytes(value.getMostSignificantBits)
                buffer ++= ByteUtil.getBytes(value.getLeastSignificantBits)
                buffer.toArray
            case value: ObjectId => value.toByteArray
            //case value: BasicBSONList => Array[Byte]()
            //case value: BSONObject => Array[Byte]()
            //case value: Pattern => Array[Byte]()
            //case value: MaxKey => Array[Byte]()
            //case value: MinKey => Array[Byte]()
            case value: CodeWScope => ByteUtil.getBytes(value.getCode)
            case value: Code => ByteUtil.getBytes(value.getCode)
            case value: Symbol => ByteUtil.getBytes(value.getSymbol)
            case _ => Array[Byte]()
        }
    }

    private def toBoolean(value: Any): Boolean = {
        value match {
            case value: java.lang.Boolean => value
            case value: java.lang.Float => value != 0
            case value: java.lang.Double => value != 0
            case value: java.lang.Byte => value != 0
            case value: java.lang.Short => value != 0
            case value: java.lang.Integer => value != 0
            case value: java.lang.Long => value != 0
            case value: java.math.BigInteger => !value.equals(BigInteger.ZERO)
            case value: java.math.BigDecimal => !value.equals(java.math.BigDecimal.ZERO)
            case value: BSONDecimal => !value.toBigDecimal.equals(java.math.BigDecimal.ZERO)
            //case value: BSONTimestamp
            //case value: java.util.Date
            //case value: String
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => false
        }
    }

    private def toByte(value: Any): Byte = {
        value match {
            case value: java.lang.Boolean => if (value) 1 else 0
            case value: java.lang.Float => value.toByte
            case value: java.lang.Double => value.toByte
            case value: java.lang.Byte => value
            case value: java.lang.Short => value.toByte
            case value: java.lang.Integer => value.toByte
            case value: java.lang.Long => value.toByte
            case value: java.math.BigInteger => value.byteValue()
            case value: java.math.BigDecimal => value.byteValue()
            case value: BSONDecimal => value.toBigDecimal.byteValue()
            case value: BSONTimestamp => (value.getTime * 1000 + value.getInc / 1000).toByte
            case value: java.util.Date => value.getTime.toByte
            case value: String => value.toByte
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => 0
        }
    }

    private def toDate(value: Any): java.sql.Date = {
        value match {
            //case value: java.lang.Boolean
            //case value: java.lang.Float
            //case value: java.lang.Double
            //case value: java.lang.Byte
            //case value: java.lang.Short
            case value: java.lang.Integer => new Date(value.toLong)
            case value: java.lang.Long => new Date(value)
            case value: java.math.BigInteger => new Date(value.longValue())
            case value: java.math.BigDecimal => new Date(value.longValue())
            case value: BSONDecimal => new Date(value.toBigDecimal.longValue())
            case value: BSONTimestamp => new Date(value.getTime.toLong * 1000 + value.getInc / 1000)
            case value: java.util.Date => new Date(value.getTime)
            case value: String =>
                new Date(new SimpleDateFormat(DATETIME_FORMAT_PATTERN).parse(value).getTime)
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => new Date(0)
        }
    }

    private def toLocalDate(value: Any): java.time.LocalDate = {
        value match {
            //case value: java.lang.Boolean
            //case value: java.lang.Float
            //case value: java.lang.Double
            //case value: java.lang.Byte
            //case value: java.lang.Short
            case value: java.lang.Integer => convertEpochMillsToLocalDate(value.toLong)
            case value: java.lang.Long => convertEpochMillsToLocalDate(value)
            case value: java.math.BigInteger => convertEpochMillsToLocalDate(value.longValue())
            case value: java.math.BigDecimal => convertEpochMillsToLocalDate(value.longValue())
            case value: BSONDecimal => convertEpochMillsToLocalDate(value.toBigDecimal.longValue())
            case value: BSONTimestamp =>
                val millsSeconds = value.getTime.toLong * 1000 + value.getInc / 1000
                convertEpochMillsToLocalDate(millsSeconds)
            case value: java.util.Date =>
                value.toInstant.atZone(ZoneId.systemDefault()).toLocalDate
            case value: String =>
                val formatter = DateTimeFormatter.ofPattern(DATETIME_FORMAT_PATTERN)
                LocalDate.parse(value, formatter)
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => EPOCH_DATE
        }
    }

    private def convertEpochMillsToLocalDate(millsSeconds: Long): LocalDate = {
        Instant.ofEpochMilli(millsSeconds)
            .atZone(ZoneId.systemDefault())
            .toLocalDate
    }

    private def toDecimal(value: Any): Decimal = {
        value match {
            case value: java.lang.Boolean => if (value) Decimal(1) else Decimal(0)
            case value: java.lang.Float => Decimal(value.toDouble)
            case value: java.lang.Double => Decimal(value)
            case value: java.lang.Byte => Decimal(value.toInt)
            case value: java.lang.Short => Decimal(value.toInt)
            case value: java.lang.Integer => Decimal(value)
            case value: java.lang.Long => Decimal(value)
            case value: java.math.BigInteger => Decimal(value)
            case value: java.math.BigDecimal => Decimal(value)
            case value: BSONDecimal => Decimal(value.toBigDecimal)
            case value: BSONTimestamp => Decimal(value.getTime.toLong * 1000 + value.getInc / 1000)
            case value: java.util.Date => Decimal(value.getTime)
            case value: String => Decimal(value)
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => Decimal(0)
        }
    }

    private def toDouble(value: Any): Double = {
        value match {
            case value: java.lang.Boolean => if (value) 1 else 0
            case value: java.lang.Float => value.toDouble
            case value: java.lang.Double => value
            case value: java.lang.Byte => value.toDouble
            case value: java.lang.Short => value.toDouble
            case value: java.lang.Integer => value.toDouble
            case value: java.lang.Long => value.toDouble
            case value: java.math.BigInteger => value.doubleValue()
            case value: java.math.BigDecimal => value.doubleValue()
            case value: BSONDecimal => value.toBigDecimal.doubleValue()
            case value: BSONTimestamp => (value.getTime.toLong * 1000 + value.getInc / 1000).toDouble
            case value: java.util.Date => value.getTime.toDouble
            case value: String => value.toDouble
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => 0
        }
    }

    private def toFloat(value: Any): Float = {
        value match {
            case value: java.lang.Boolean => if (value) 1 else 0
            case value: java.lang.Float => value
            case value: java.lang.Double => value.toFloat
            case value: java.lang.Byte => value.toFloat
            case value: java.lang.Short => value.toFloat
            case value: java.lang.Integer => value.toFloat
            case value: java.lang.Long => value.toFloat
            case value: java.math.BigInteger => value.floatValue()
            case value: java.math.BigDecimal => value.floatValue()
            case value: BSONDecimal => value.toBigDecimal.floatValue()
            case value: BSONTimestamp => (value.getTime.toLong * 1000 + value.getInc / 1000).toFloat
            case value: java.util.Date => value.getTime.toFloat
            case value: String => value.toFloat
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => 0
        }
    }

    private def toInt(value: Any): Int = {
        value match {
            case value: java.lang.Boolean => if (value) 1 else 0
            case value: java.lang.Float => value.toInt
            case value: java.lang.Double => value.toInt
            case value: java.lang.Byte => value.toInt
            case value: java.lang.Short => value.toInt
            case value: java.lang.Integer => value
            case value: java.lang.Long => value.toInt
            case value: java.math.BigInteger => value.intValue()
            case value: java.math.BigDecimal => value.intValue()
            case value: BSONDecimal => value.toBigDecimal.intValue()
            case value: BSONTimestamp => (value.getTime.toLong * 1000 + value.getInc / 1000).toInt
            case value: java.util.Date => value.getTime.toInt
            case value: String => value.toInt
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => 0
        }
    }

    private def toLong(value: Any): Long = {
        value match {
            case value: java.lang.Boolean => if (value) 1 else 0
            case value: java.lang.Float => value.toLong
            case value: java.lang.Double => value.toLong
            case value: java.lang.Byte => value.toLong
            case value: java.lang.Short => value.toLong
            case value: java.lang.Integer => value.toLong
            case value: java.lang.Long => value
            case value: java.math.BigInteger => value.longValue()
            case value: java.math.BigDecimal => value.longValue()
            case value: BSONDecimal => value.toBigDecimal.longValue()
            case value: BSONTimestamp => value.getTime.toLong * 1000 + value.getInc / 1000
            case value: java.util.Date => value.getTime
            case value: String => value.toLong
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => 0
        }
    }

    private def toShort(value: Any): Short = {
        value match {
            case value: java.lang.Boolean => if (value) 1 else 0
            case value: java.lang.Float => value.toShort
            case value: java.lang.Double => value.toShort
            case value: java.lang.Byte => value.toShort
            case value: java.lang.Short => value
            case value: java.lang.Integer => value.toShort
            case value: java.lang.Long => value.toShort
            case value: java.math.BigInteger => value.shortValue()
            case value: java.math.BigDecimal => value.shortValue()
            case value: BSONDecimal => value.toBigDecimal.shortValue()
            case value: BSONTimestamp => (value.getTime.toLong * 1000 + value.getInc / 1000).toShort
            case value: java.util.Date => value.getTime.toShort
            case value: String => value.toShort
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => 0
        }
    }

    private def toString(value: Any): String = {
        value match {
            //case value: java.lang.Boolean => value.toString
            //case value: java.lang.Float => value.toString
            //case value: java.lang.Double => value.toString
            //case value: java.lang.Byte => value.toString
            //case value: java.lang.Short => value.toString
            //case value: java.lang.Integer => value.toString
            //case value: java.lang.Long => value.toString
            //case value: java.math.BigInteger => value.toString
            //case value: java.math.BigDecimal => value.toString
            case value: BSONDecimal => value.getValue
            case value: BSONTimestamp =>
                val ts = new Timestamp(value.getTime.toLong * 1000)
                ts.setNanos(value.getInc * 1000)
                ts.toString
            case value: java.util.Date => new Date(value.getTime).toString
            case value: String => value
            case value: Binary => new String(value.getData, "UTF-8")
            //case value: UUID => value.toString
            //case value: ObjectId => value.toString
            //case value: BasicBSONList => value.toString
            //case value: BSONObject => value.toString
            //case value: Pattern => value.toString
            //case value: MaxKey => value.toString
            //case value: MinKey => value.toString
            //case value: Code => value.toString
            //case value: CodeWScope => value.toString
            //case value: Symbol => value.toString
            case _ => value.toString
        }
    }

    private def toTimestamp(value: Any): java.sql.Timestamp = {
        value match {
            //case value: java.lang.Boolean
            //case value: java.lang.Float
            //case value: java.lang.Double
            //case value: java.lang.Byte
            //case value: java.lang.Short
            case value: java.lang.Integer => new Timestamp(value.toLong)
            case value: java.lang.Long => new Timestamp(value)
            case value: java.math.BigInteger => new Timestamp(value.longValue())
            case value: java.math.BigDecimal => new Timestamp(value.longValue())
            case value: BSONDecimal => new Timestamp(value.toBigDecimal.longValue())
            case value: BSONTimestamp =>
                val ts = new Timestamp(value.getTime.toLong * 1000)
                ts.setNanos(value.getInc * 1000)
                ts
            case value: java.util.Date => new Timestamp(value.getTime)
            case value: String =>
                new Timestamp(new SimpleDateFormat(DATETIME_FORMAT_PATTERN).parse(value).getTime)
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => new Timestamp(0)
        }
    }

    private def toInstant(value: Any): java.time.Instant = {
        value match {
            //case value: java.lang.Boolean
            //case value: java.lang.Float
            //case value: java.lang.Double
            //case value: java.lang.Byte
            //case value: java.lang.Short
            case value: java.lang.Integer => toTimestamp(value).toInstant
            case value: java.lang.Long => toTimestamp(value).toInstant
            case value: java.math.BigInteger => toTimestamp(value).toInstant
            case value: java.math.BigDecimal => toTimestamp(value).toInstant
            case value: BSONDecimal => toTimestamp(value).toInstant
            case value: BSONTimestamp => toTimestamp(value).toInstant
            case value: java.util.Date => value.toInstant
            case value: String =>
                val formatter = DateTimeFormatter.ofPattern(DATETIME_FORMAT_PATTERN)
                LocalDateTime.parse(value, formatter)
                    .atZone(ZoneId.systemDefault())
                    .toInstant
            //case value: Binary
            //case value: UUID
            //case value: ObjectId
            //case value: BasicBSONList
            //case value: BSONObject
            //case value: Pattern
            //case value: MaxKey
            //case value: MinKey
            //case value: Code
            //case value: CodeWScope
            //case value: Symbol
            case _ => Instant.EPOCH
        }
    }

    // data is BSONObject filed which get from SequoiaDB,
    // so it's type is limited
    def typeOfData(data: Any): DataType = data match {
        case _: Boolean => BooleanType
        case _: Float => FloatType
        case _: Double => DoubleType
        case _: Byte => ByteType
        case _: Short => ShortType
        case _: Int => IntegerType
        case _: Long => LongType
        // DecimalType can only support precision up to 38
        case _: java.math.BigInteger => BigIntDecimal
        case d: java.math.BigDecimal =>
            DecimalType(
                min(d.precision, DecimalType.MAX_PRECISION),
                min(d.scale, DecimalType.MAX_SCALE)
            )
        case b: BSONDecimal =>
            val d = b.toBigDecimal
            DecimalType(
                min(d.precision, DecimalType.MAX_PRECISION),
                min(d.scale, DecimalType.MAX_SCALE)
            )
        case _: BSONTimestamp => TimestampType
        case _: java.util.Date => DateType
        case _: String => StringType
        case _: Binary => BinaryType
        case _: UUID => BinaryType
        case _: ObjectId => StringType
        case array: BasicBSONList => typeOfArray(array)
        case obj: BSONObject =>
            val fields = obj.toMap.map {
                case (k, v) => StructField(k.asInstanceOf[String], typeOfData(v))
            }.toSeq
            StructType(fields)
        //case _: Pattern
        //case _: MaxKey
        //case _: MinKey
        //case _: Code
        //case _: CodeWScope
        //case _: Symbol
        case null => NullType
        case _ => StringType
    }

    // The decimal types compatible with other numeric types
    private[spark] val ByteDecimal = DecimalType(3, 0)
    private[spark] val ShortDecimal = DecimalType(5, 0)
    private[spark] val IntDecimal = DecimalType(10, 0)
    private[spark] val LongDecimal = DecimalType(20, 0)
    private[spark] val FloatDecimal = DecimalType(14, 7)
    private[spark] val DoubleDecimal = DecimalType(30, 15)
    private[spark] val BigIntDecimal = DecimalType(38, 0)

    private lazy val findTightestCommonType: (DataType, DataType) => Option[DataType] = {
        val cls = TypeCoercion.getClass
        val f: Field = {
            try {
                cls.getDeclaredField("findTightestCommonType")
            } catch {
                case _: NoSuchFieldException => {
                    cls.getDeclaredField("findTightestCommonTypeOfTwo")
                }
                case e: Throwable => throw e
            }
        }

        f.setAccessible(true)
        try {
            f.get(TypeCoercion)
                .asInstanceOf[(DataType, DataType) => Option[DataType]]
        } finally {
            f.setAccessible(false)
        }
    }

    private[spark] def forType(dataType: DataType): DecimalType = dataType match {
        case ByteType => ByteDecimal
        case ShortType => ShortDecimal
        case IntegerType => IntDecimal
        case LongType => LongDecimal
        case FloatType => FloatDecimal
        case DoubleType => DoubleDecimal
    }

    /**
      * It looks for the most compatible type between two given DataTypes.
      * i.e.: {{{
      *   val dataType1 = IntegerType
      *   val dataType2 = DoubleType
      *   assert(compatibleType(dataType1,dataType2)==DoubleType)
      * }}}
      */
    def compatibleType(t1: DataType, t2: DataType): DataType = {
        findTightestCommonType(t1, t2) match {
            case Some(commonType) => commonType
            case None =>
                // t1 or t2 is a StructType, ArrayType, or an unexpected type.
                (t1, t2) match {
                    case (other: DataType, NullType) => other
                    case (NullType, other: DataType) => other
                    case (t1: DecimalType, t2: DecimalType) =>
                        DecimalPrecision.widerDecimalType(t1, t2)
                    case (t: NumericType, d: DecimalType) =>
                        DecimalPrecision.widerDecimalType(forType(t), d)
                    case (d: DecimalType, t: NumericType) =>
                        DecimalPrecision.widerDecimalType(forType(t), d)
                    case (StructType(fields1), StructType(fields2)) =>
                        val newFields = (fields1 ++ fields2)
                            .groupBy(field => field.name)
                            .map { case (name, fieldTypes) =>
                                val dataType = fieldTypes
                                    .map(field => field.dataType)
                                    .reduce(compatibleType)
                                StructField(name, dataType, nullable = true)
                            }
                        StructType(newFields.toSeq.sortBy(_.name))
                    case (ArrayType(elementType1, containsNull1), ArrayType(elementType2, containsNull2)) =>
                        ArrayType(compatibleType(elementType1, elementType2),
                            containsNull1 || containsNull2)
                    case (_, _) => StringType
                }
        }
    }

    def typeOfArray(l: Seq[Any]): ArrayType = {
        val containsNull = l.contains(null)
        val elements = l.flatMap(v => Option(v))
        if (elements.isEmpty) {
            // If this JSON array is empty, we use NullType as a placeholder.
            // If this array is not empty in other JSON objects, we can resolve
            // the type after we have passed through all JSON objects.
            ArrayType(NullType, containsNull)
        } else {
            val elementType = elements
                .map(typeOfData)
                .reduce(compatibleType)
            ArrayType(elementType, containsNull)
        }
    }

    def removeNullFields(array: BasicBSONList): BasicBSONList = {
        if (array == null) {
            return null
        }

        // when remove field in BasicBSONList,
        // the elements behind the removed one will be moved forward,
        // and keys behind the removed one will be changed,
        // so can not use keySet like BSONObject
        val iter = array.iterator()
        while (iter.hasNext) {
            val value = iter.next()
            value match {
                case null => iter.remove()
                case value: BasicBSONList =>
                    val embeddedArray = removeNullFields(value)
                    if (embeddedArray == null) {
                        iter.remove()
                    }
                case value: BSONObject =>
                    val embeddedObj = removeNullFields(value)
                    if (embeddedObj == null) {
                        iter.remove()
                    }
                case _ =>
            }
        }

        if (array.isEmpty) {
            null
        } else {
            array
        }
    }

    def removeNullFields(obj: BSONObject): BSONObject = {
        if (obj == null) {
            return null
        }

        // keySet actually hold a iterator of obj,
        // exception will be thrown when remove filed,
        // so we convert it to Sequence
        obj.keySet().toSeq.foreach { field =>
            val value = obj.get(field)
            value match {
                case null => obj.removeField(field)
                case value: BasicBSONList =>
                    val embeddedArray = removeNullFields(value)
                    if (embeddedArray == null) {
                        obj.removeField(field)
                    }
                case value: BSONObject =>
                    val embeddedObj = removeNullFields(value)
                    if (embeddedObj == null) {
                        obj.removeField(field)
                    }
                case _ =>
            }
        }

        if (obj.isEmpty) {
            null
        } else {
            obj
        }
    }
}
