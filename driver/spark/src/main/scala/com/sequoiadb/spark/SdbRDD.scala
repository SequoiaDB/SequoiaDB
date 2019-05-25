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

import java.net.InetAddress

import org.apache.spark.annotation.DeveloperApi
import org.apache.spark.rdd.RDD
import org.apache.spark.sql.Row
import org.apache.spark.sql.sources.Filter
import org.apache.spark.sql.types.StructType
import org.apache.spark.{Partition, SparkContext, TaskContext}
import org.bson.BSONObject

import scala.collection.mutable.ArrayBuffer
import scala.reflect.ClassTag

/**
  * SequoiaDB RDD
  *
  * @param sc              SparkContext
  * @param config          configurations
  * @param requiredColumns query selector
  * @param filter          query filter
  */
abstract class SdbRDD[T: ClassTag](sc: SparkContext,
                                   config: SdbConfig,
                                   requiredColumns: Array[String] = Array(),
                                   filter: SdbFilter = SdbFilter())
    extends RDD[T](sc, Nil) {

    logInfo(s"SdbRDD{config: $config, filter: $filter, selector: [${requiredColumns.mkString(", ")}]}")

    @DeveloperApi
    override def compute(split: Partition, context: TaskContext): Iterator[T] = {
        val iterator = createIterator(split.asInstanceOf[SdbPartition])
        context.addTaskCompletionListener(_ => iterator.close())
        context.addTaskFailureListener((_, _) => iterator.close())
        iterator
    }

    override protected def getPartitions: Array[Partition] = {
        val partitions = SdbPartitioner(config, filter).computePartitions()
            .asInstanceOf[Array[Partition]]
        partitions.foreach(partition => logInfo(partition.toString))
        partitions
    }

    override def getPreferredLocations(split: Partition): Seq[String] = {
        if (config.preferredLocation) {
            val hosts = ArrayBuffer[String]()
            val urls = split.asInstanceOf[SdbPartition].urls
            urls.foreach { url =>
                val host = url.split(':')(0).trim
                val ip = InetAddress.getByName(host).getHostAddress
                hosts += ip
            }
            hosts.distinct
        } else {
            Nil
        }
    }

    protected def createIterator(sdbPartition: SdbPartition): SdbRDDIterator[T]
}

/**
  * SequoiaDB RDD of Row
  *
  * @param sc              SparkContext
  * @param config          configurations
  * @param schema          the schema of Row
  * @param requiredColumns query selector
  * @param filter          query filter
  */
class SdbRowRDD(sc: SparkContext,
                config: SdbConfig,
                schema: StructType,
                requiredColumns: Array[String] = Array(),
                filter: SdbFilter = SdbFilter())
    extends SdbRDD[Row](sc, config, requiredColumns, filter) {

    override def createIterator(sdbPartition: SdbPartition): SdbRDDIterator[Row] = {
        new SdbRowRDDIterator(config, sdbPartition, schema, requiredColumns)
    }
}

object SdbRowRDD {
    def apply(sc: SparkContext, sdbConfig: SdbConfig, schema: StructType): SdbRowRDD = {
        new SdbRowRDD(sc, sdbConfig, schema)
    }

    def apply(sc: SparkContext,
              sdbConfig: SdbConfig,
              schema: StructType,
              requiredColumns: Array[String]): SdbRowRDD = {
        new SdbRowRDD(sc, sdbConfig, schema, requiredColumns)
    }

    def apply(sc: SparkContext,
              sdbConfig: SdbConfig,
              schema: StructType,
              requiredColumns: Array[String] = Array(),
              filters: Array[Filter] = Array()): SdbRowRDD = {
        new SdbRowRDD(sc, sdbConfig, schema, requiredColumns, SdbFilter(filters))
    }
}

/**
  * SequoiaDB RDD of BSONObject
  *
  * @param sc              SparkContext
  * @param sdbConfig       configurations
  * @param requiredColumns query selector
  * @param filter          query filter
  * @param numReturned     query returned num
  */
class SdbBsonRDD(sc: SparkContext,
                 sdbConfig: SdbConfig,
                 requiredColumns: Array[String] = Array(),
                 filter: SdbFilter = SdbFilter(),
                 numReturned: Long = -1)
    extends SdbRDD[BSONObject](sc, sdbConfig, requiredColumns, filter) {

    override def createIterator(sdbPartition: SdbPartition): SdbRDDIterator[BSONObject] = {
        new SdbBsonRDDIterator(sdbConfig, sdbPartition, requiredColumns, numReturned)
    }
}

/**
  * RDD[BSONObject] functions
  *
  * @param rdd RDD[BSONObject]
  */
class SdbBsonRDDFunctions(rdd: RDD[BSONObject]) {

    private def saveToSequoiadb(properties: Map[String, String]): Unit = {
        val config = SdbConfig(properties)

        rdd.foreachPartition { it =>
            new SdbWriter(config).write(it)
        }
    }

    def saveToSequoiadb(host: String,
                        csName: String,
                        clName: String,
                        username: String = "",
                        password: String = "",
                        bulkSize: Int = SdbConfig.DefaultBulkSize): Unit = {
        if (host == null || host.isEmpty) {
            throw new SdbException("Invalid host")
        }
        if (csName == null || csName.isEmpty) {
            throw new SdbException("Invalid csName")
        }
        if (clName == null || clName.isEmpty) {
            throw new SdbException("Invalid clName")
        }
        if (bulkSize <= 0) {
            throw new SdbException(s"Invalid bulkSize: $bulkSize")
        }

        val config = Map(
            (SdbConfig.Host, host),
            (SdbConfig.CollectionSpace, csName),
            (SdbConfig.Collection, clName),
            (SdbConfig.Username, username),
            (SdbConfig.Password, password),
            (SdbConfig.BulkSize, bulkSize.toString)
        )

        saveToSequoiadb(config)
    }
}
