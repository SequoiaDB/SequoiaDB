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

import org.apache.spark.rdd.RDD
import org.apache.spark.{SparkConf, SparkContext}
import org.bson.{BSONObject, BasicBSONObject}
import org.bson.util.JSON

import scala.collection.mutable

class SparkContextFunctions(sc: SparkContext) {

    private val PREFIX = "sequoiadb."

    private def buildConf(conf: SparkConf,
                          parameters: Map[String, String])
    : Map[String, String] = {
        val allConfigs = SdbConfig.AllProperties
        val newConf = mutable.HashMap[String, String]()

        allConfigs.foreach { name: String =>
            val value = conf.getOption(PREFIX + name)
            if (value.nonEmpty) {
                newConf += (name -> value.get)
            }
        }

        if (!newConf.contains(SdbConfig.Host)) {
            throw new SdbException("Missing parameter \"" + PREFIX + SdbConfig.Host + "\"")
        }

        // the same SparkContext connect to the same SequoiaDB cluster
        // can't override connection info of SparkConf through parameters
        val params = parameters -- Seq(SdbConfig.Host, SdbConfig.Username, SdbConfig.Password)

        newConf ++= params
        newConf.toMap
    }

    def loadFromSequoiadb(csName: String,
                          clName: String,
                          matcher: String = "{}",
                          parameters: Map[String, String] = Map())
    : RDD[BSONObject] = {
        if (csName == null || csName.isEmpty) {
            throw new SdbException("Invalid csName")
        }
        if (clName == null || clName.isEmpty) {
            throw new SdbException("Invalid clName")
        }
        val collectionMap = Map(
            (SdbConfig.CollectionSpace, csName),
            (SdbConfig.Collection, clName))
        val newParameters = if (parameters == null) collectionMap else parameters ++ collectionMap
        val conf = buildConf(sc.getConf, newParameters)
        val matcherObj = Option(JSON.parse(matcher).asInstanceOf[BSONObject]).getOrElse(new BasicBSONObject())
        new SdbBsonRDD(sc, SdbConfig(conf), filter = SdbFilter(matcherObj))
    }
}
