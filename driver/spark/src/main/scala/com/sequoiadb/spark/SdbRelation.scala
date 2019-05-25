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

import com.sequoiadb.base.Sequoiadb
import org.apache.spark.rdd.RDD
import org.apache.spark.sql.sources._
import org.apache.spark.sql.types.StructType
import org.apache.spark.sql.{DataFrame, Row, SQLContext}

import scala.collection.JavaConversions._

/**
  * SequoiaDB Relation by collection mapping
  *
  * @param sqlContext     SQLContext
  * @param config         configurations
  * @param providedSchema row schema
  */
class SdbRelation(@transient val sqlContext: SQLContext,
                  val config: SdbConfig,
                  val providedSchema: Option[StructType] = None)
    extends BaseRelation
        with TableScan
        with PrunedScan
        with PrunedFilteredScan
        with InsertableRelation
        with Logging
        with Serializable {

    logInfo(s"SdbRelation{config: $config, providedSchema: $providedSchema}")

    private lazy val lazySchema: StructType = {
        val conf: SdbConfig = if (config.samplingSingle) {
            val props = config.properties +
                (SdbConfig.PartitionMode -> SdbConfig.PARTITION_MODE_SINGLE)
            SdbConfig(props)
        } else {
            config
        }
        val rdd = new SdbBsonRDD(sqlContext.sparkContext, conf,
            numReturned = config.samplingNum)
        new SdbSchemaSampler(rdd, config.samplingRatio, config.samplingWithId).sample()
    }

    override def schema: StructType = providedSchema.getOrElse(lazySchema)

    if (config.useSelector == SdbConfig.USE_SELECTOR_AUTO) {
        logInfo(s"sparksql-schema=$schema, sequoiadb-schema=$lazySchema")
    }

    private def needSelector(targetSchema: StructType): Boolean = {
        config.useSelector.toLowerCase match {
            case SdbConfig.USE_SELECTOR_ENABLE => true
            case SdbConfig.USE_SELECTOR_DISABLE => false
            case _ =>
                if (targetSchema.equals(lazySchema)) {
                    false
                } else if (targetSchema.size >= lazySchema.size - config.selectorDiff) {
                    false
                } else {
                    true
                }
        }
    }

    private def realColumns(targetSchema: StructType,
                            requiredColumns: Array[String])
    : Array[String] = {
        if (requiredColumns.isEmpty) {
            return requiredColumns
        }

        if (needSelector(targetSchema)) {
            requiredColumns
        } else {
            Array()
        }
    }

    override def buildScan(): RDD[Row] = {
        logInfo(s"select * from ${config.collectionSpace}.${config.collection}")
        SdbRowRDD(sqlContext.sparkContext, config, schema)
    }

    override def buildScan(requiredColumns: Array[String]): RDD[Row] = {
        logInfo(s"select ${requiredColumns.mkString(", ")} " +
            s"from ${config.collectionSpace}.${config.collection}")
        val prunedSchema = SdbRelation.pruneSchema(schema, requiredColumns)
        SdbRowRDD(sqlContext.sparkContext, config,
            prunedSchema, realColumns(prunedSchema, requiredColumns))
    }

    override def buildScan(requiredColumns: Array[String], filters: Array[Filter]): RDD[Row] = {
        logInfo(s"select ${requiredColumns.mkString(", ")} " +
            s"from ${config.collectionSpace}.${config.collection} " +
            s"where ${filters.mkString(", ")}")
        val prunedSchema = SdbRelation.pruneSchema(schema, requiredColumns)
        SdbRowRDD(sqlContext.sparkContext, config,
            prunedSchema, realColumns(prunedSchema, requiredColumns), filters)
    }

    override def unhandledFilters(filters: Array[Filter]): Array[Filter] = {
        SdbFilter(filters).unhandledFilters()
    }

    override def insert(data: DataFrame, overwrite: Boolean): Unit = {
        logInfo(s"insert into ${config.collectionSpace}.${config.collection}")

        if (overwrite) {
            val sdb = new Sequoiadb(config.host, config.username, config.password, null)
            try {
                val cs = if (sdb.isCollectionSpaceExist(config.collectionSpace)) {
                    sdb.getCollectionSpace(config.collectionSpace)
                } else {
                    throw new SdbException(s"Collection space is not existing: ${config.collectionSpace}")
                }

                val cl = if (cs.isCollectionExist(config.collection)) {
                    cs.getCollection(config.collection)
                } else {
                    throw new SdbException(s"Collection is not existing: " +
                        s"${config.collectionSpace}.${config.collection}")
                }

                cl.truncate()
            } finally {
                sdb.disconnect()
            }
        }

        data.foreachPartition(it => {
            // always write through coordinator node which specified in config
            new SdbWriter(config).write(it, schema)
        })

        logInfo(s"finished inserting into ${config.collectionSpace}.${config.collection}")
    }
}

object SdbRelation {
    def apply(sqlContext: SQLContext,
              sdbConfig: SdbConfig,
              providedSchema: Option[StructType] = None): SdbRelation = {
        new SdbRelation(sqlContext, sdbConfig, providedSchema)
    }

    /**
      * Prune whole schema in order to fit with
      * required columns in Spark SQL statement.
      *
      * @param schema          Whole field projection schema.
      * @param requiredColumns Required fields in statement
      * @return A new pruned schema
      */
    private def pruneSchema(schema: StructType, requiredColumns: Array[String]): StructType = {
        StructType(
            requiredColumns.flatMap {
                column => schema.fields.find(_.name == column)
            }
        )
    }
}
