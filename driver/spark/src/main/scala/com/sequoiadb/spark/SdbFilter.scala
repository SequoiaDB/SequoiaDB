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

import java.util.regex.Pattern

import org.apache.spark.sql.sources._
import org.bson.types.BasicBSONList
import org.bson.{BSON, BSONObject, BasicBSONObject}

import scala.collection.JavaConversions._
import scala.collection.mutable.ArrayBuffer

/**
  * SequoiaDB query filter
  */
class SdbFilter extends Serializable {

    // BSONObject is not serializable, so we convert it to string
    private var matcher: Array[Byte] = BSON.encode(new BasicBSONObject())
    private var unhandled: Array[Filter] = Array()

    /**
      * Create SequoiaDB query filter by SparkSQL filter
      *
      * @param filters SparkSQL filters
      */
    def this(filters: Array[Filter]) {
        this()
        val (m, u) = SdbFilter.toBSONObj(filters)
        this.matcher = BSON.encode(m)
        this.unhandled = u
    }

    /**
      * Create SequiaDB query filter by SequoiaDB matcher BSONObject
      *
      * @param matcher SequoiaDB matcher BSONObject
      */
    def this(matcher: BSONObject) {
        this()
        if (matcher != null) {
            this.matcher = BSON.encode(matcher)
        }
    }

    def BSONObj(): BSONObject = BSON.decode(matcher)

    def unhandledFilters(): Array[Filter] = unhandled

    override def toString: String = BSONObj().toString
}

object SdbFilter {
    val AND = "$and"
    val OR = "$or"
    val NOT = "$not"
    val ET = "$et"
    val GT = "$gt"
    val GTE = "$gte"
    val IN = "$in"
    val LT = "$lt"
    val LTE = "$lte"
    val ISNULL = "$isnull"

    def apply(): SdbFilter = new SdbFilter()

    def apply(filters: Array[Filter]): SdbFilter = new SdbFilter(filters)

    def apply(matcher: BSONObject): SdbFilter = new SdbFilter(matcher)

    // NOT for And & Or filters
    private def appendFilter(obj: BSONObject, attribute: String, subObj: Any): BSONObject = {
        var newObj = obj

        if (obj.containsField(AND)) {
            val element = new BasicBSONObject()
            element.put(attribute, subObj)

            val andObj = obj.get(AND).asInstanceOf[BasicBSONList]
            andObj.add(element)
        } else if (obj.containsField(attribute)) {
            val element: BSONObject = new BasicBSONObject()
            element.put(attribute, subObj)

            val array = new BasicBSONList()
            obj.toMap.foreach {
                case (key: String, value) =>
                    val eachObj = new BasicBSONObject()
                    eachObj.put(key, value)
                    array.add(eachObj)
            }
            array.add(element)

            val andObj = new BasicBSONObject()
            andObj.put(AND, array)

            newObj = andObj
        } else {
            obj.put(attribute, subObj)
        }

        newObj
    }

    // only for And Filter
    private def appendAndFilter(left: BSONObject, right: BSONObject): BSONObject = {
        var obj: BSONObject = new BasicBSONObject()

        if (left.containsField(AND) && right.containsField(AND)) {
            val newArray = new BasicBSONList()
            newArray.addAll(left.get(AND).asInstanceOf[BasicBSONList])
            newArray.addAll(right.get(AND).asInstanceOf[BasicBSONList])

            obj.put(AND, newArray)
        } else if (left.containsField(AND) && !right.containsField(AND)) {
            obj.putAll(left)
            right.toMap.foreach {
                case (key: String, value) => obj = appendFilter(obj, key, value)
            }
        } else if (!left.containsField(AND) && right.containsField(AND)) {
            obj.putAll(right)
            left.toMap.foreach {
                case (key: String, value) => obj = appendFilter(obj, key, value)
            }
        } else {
            val newArray = new BasicBSONList()
            newArray.add(left)
            newArray.add(right)

            obj.put(AND, newArray)
        }

        obj
    }

    // only for Or filter
    private def appendOrFilter(left: BSONObject, right: BSONObject): BSONObject = {
        var obj: BSONObject = new BasicBSONObject()

        if (left.containsField(OR) && right.containsField(OR)) {
            val newArray = new BasicBSONList()
            newArray.addAll(left.get(OR).asInstanceOf[BasicBSONList])
            newArray.addAll(right.get(OR).asInstanceOf[BasicBSONList])

            obj.put(OR, newArray)
        } else if (left.containsField(OR) && !right.containsField(OR)) {
            val newArray = left.get(OR).asInstanceOf[BasicBSONList]
            newArray.add(right)
            obj = left
            obj.put(OR, newArray)
        } else if (!left.containsField(OR) && right.containsField(OR)) {
            val newArray = right.get(OR).asInstanceOf[BasicBSONList]
            newArray.add(left)
            obj = right
            obj.put(OR, newArray)
        } else {
            val newArray = new BasicBSONList()
            newArray.add(left)
            newArray.add(right)

            obj.put(OR, newArray)
        }

        obj
    }

    // convert Spark filters to SequoiaDB matcher BSONObject
    private def toBSONObj(filters: Array[Filter] = Array()): (BSONObject, Array[Filter]) = {
        var matcher: BSONObject = new BasicBSONObject()
        val unhandled = ArrayBuffer[Filter]()

        filters.foreach {
            case And(left, right) =>
                val (leftObj, _) = toBSONObj(Array(left))
                val (rightObj, _) = toBSONObj(Array(right))
                val andObj = appendAndFilter(leftObj, rightObj)
                if (matcher.isEmpty) {
                    matcher = andObj
                } else {
                    matcher = appendAndFilter(matcher, andObj)
                }
            case EqualNullSafe(attribute, value) =>
                val obj = new BasicBSONObject()
                obj.put(ET, value)
                matcher = appendFilter(matcher, attribute, obj)
            case EqualTo(attribute, value) =>
                val obj = new BasicBSONObject()
                obj.put(ET, value)
                matcher = appendFilter(matcher, attribute, obj)
            case GreaterThan(attribute, value) =>
                val obj = new BasicBSONObject()
                obj.put(GT, value)
                matcher = appendFilter(matcher, attribute, obj)
            case GreaterThanOrEqual(attribute, value) =>
                val obj = new BasicBSONObject()
                obj.put(GTE, value)
                matcher = appendFilter(matcher, attribute, obj)
            case In(attribute, values) =>
                val array = new BasicBSONList()
                Array.tabulate(values.length) { i => array.put(i, values(i)) }

                val subObj = new BasicBSONObject()
                subObj.put(IN, array)

                matcher = appendFilter(matcher, attribute, subObj)
            case IsNotNull(attribute) =>
                val subObj = new BasicBSONObject()
                subObj.put(ISNULL, 0)
                matcher = appendFilter(matcher, attribute, subObj)
            case IsNull(attribute) =>
                val subObj = new BasicBSONObject()
                subObj.put(ISNULL, 1)
                matcher = appendFilter(matcher, attribute, subObj)
            case LessThan(attribute, value) =>
                val subObj = new BasicBSONObject()
                subObj.put(LT, value)
                matcher = appendFilter(matcher, attribute, subObj)
            case LessThanOrEqual(attribute, value) =>
                val subObj = new BasicBSONObject()
                subObj.put(LTE, value)
                matcher = appendFilter(matcher, attribute, subObj)
            case Not(child) =>
                val (notObj, _) = toBSONObj(Array(child))
                val array = new BasicBSONList()
                array.put(0, notObj)
                matcher = appendFilter(matcher, NOT, array)
            case Or(left, right) =>
                val (leftObj, _) = toBSONObj(Array(left))
                val (rightObj, _) = toBSONObj(Array(right))
                val orObj = appendOrFilter(leftObj, rightObj)
                if (matcher.isEmpty) {
                    matcher = orObj
                } else {
                    matcher = appendAndFilter(matcher, orObj)
                }
            case StringContains(attribute, value) =>
                val subObj = Pattern.compile(".*" + value + ".*")
                matcher = appendFilter(matcher, attribute, subObj)
            case StringEndsWith(attribute, value) =>
                val subObj = Pattern.compile(".*" + value + "$")
                matcher = appendFilter(matcher, attribute, subObj)
            case StringStartsWith(attribute, value) =>
                val subObj = Pattern.compile("^" + value + ".*")
                matcher = appendFilter(matcher, attribute, subObj)
            case filter: Filter =>
                unhandled += filter
        }

        (matcher, unhandled.toArray)
    }
}