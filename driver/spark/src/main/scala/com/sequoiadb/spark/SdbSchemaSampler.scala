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

import org.apache.spark.sql.types._

import scala.collection.JavaConversions._

/**
  * SequoiaDB schema sampler
  *
  * @param sdbRDD         SequoiaDB RDD[BSONObject]
  * @param samplingRatio  sampling ratio
  * @param samplingWithId whether sampling with _id field
  */
private[spark] class SdbSchemaSampler(sdbRDD: SdbBsonRDD,
                                      samplingRatio: Double,
                                      samplingWithId: Boolean)
    extends Serializable with Logging {

    def sample(): StructType = {
        val schemaData = {
            if (samplingRatio > 0.99) {
                sdbRDD
            } else {
                sdbRDD.sample(withReplacement = false, samplingRatio)
            }
        }

        val structFields = schemaData.flatMap { obj =>
            if (!samplingWithId && obj.containsField("_id")) {
                obj.removeField("_id")
            }
            obj.keySet().map { key =>
                val value = obj.get(key)
                key -> BSONConverter.typeOfData(value)
            }
        }.reduceByKey(BSONConverter.compatibleType).aggregate(Seq[StructField]())(
            (fields, newField) => fields :+ StructField(newField._1, newField._2),
            (oldFields, newFields) => oldFields ++ newFields)

        val schema = StructType(structFields)
        logInfo(s"sampled schema: $schema")
        schema
    }
}
