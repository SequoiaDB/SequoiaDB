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

import com.sequoiadb.base.{DBCollection, Sequoiadb}
import com.sequoiadb.spark.SdbConfig.mergeGlobalConfs
import org.apache.spark.sql.sources._
import org.apache.spark.sql.types.StructType
import org.apache.spark.sql.{DataFrame, SQLContext, SaveMode}
import org.bson.{BSONObject, BasicBSONObject}
import org.bson.util.JSON

import scala.collection.JavaConversions._

/**
  * Default source is loaded by Spark.
  * Allows creation of SequoiaDB based tables using
  * the syntax CREATE [TEMPORARY] TABLE ... USING com.sequoiadb.spark OPTIONS(...)
  * Required options are detailed in [[com.sequoiadb.spark.SdbConfig]]
  */
class DefaultSource extends DataSourceRegister
    with RelationProvider
    with SchemaRelationProvider
    with CreatableRelationProvider
    with Logging {

    override def shortName(): String = "sequoiadb"

    /**
      * Create relation without providing schema
      */
    override def createRelation(sqlContext: SQLContext,
                                parameters: Map[String, String]): BaseRelation = {
        SdbRelation(sqlContext, parameters)
    }

    /**
      * Create relation with providing schema
      */
    override def createRelation(sqlContext: SQLContext,
                                parameters: Map[String, String],
                                schema: StructType): BaseRelation = {
        SdbRelation(sqlContext, parameters, Option(schema))
    }

    /**
      * Creatable relation
      */
    override def createRelation(sqlContext: SQLContext,
                                mode: SaveMode,
                                parameters: Map[String, String],
                                data: DataFrame): BaseRelation = {
        val config = SdbConfig(sqlContext.getAllConfs, parameters)

        // if it return true, that means we should write the data into collection
        if (isCollectionWritable(config, mode)) {
            // get schema for execution
            val schema = data.schema
            val sourceInfo = SdbConnUtil.generateSourceInfo(sqlContext.sparkContext)

            data.foreachPartition(it => {
                // always write through coord node which specified in config
                new SdbWriter(config, sourceInfo).write(it, schema)
            })
        }

        SdbRelation(sqlContext, parameters, Option(data.schema))
    }

    // Check whether a collection is writable for the given mode
    // Return true for writable, return false for non-writable, throw exception for error
    private def isCollectionWritable(config: SdbConfig, mode: SaveMode): Boolean = {
        val sdb = new Sequoiadb(config.host, config.username, config.password, SdbConfig.SdbConnectionOptions)

        try {
            mode match {
                case SaveMode.Append =>
                    ensureCollection(sdb, config.collectionSpace, config.collection, Option(config))
                    true
                case SaveMode.Overwrite =>
                    val cl = ensureCollection(sdb, config.collectionSpace, config.collection, Option(config))
                    cl.truncate()
                    true
                case SaveMode.ErrorIfExists =>
                    if (isCollectionExist(sdb, config.collectionSpace, config.collection)) {
                        throw new SdbException(
                            String.format("Collection [%s.%s] is exist",
                                config.collectionSpace, config.collection)
                        )
                    }
                    ensureCollection(sdb, config.collectionSpace, config.collection, Option(config))
                    true
                case SaveMode.Ignore =>
                    if (isCollectionExist(sdb, config.collectionSpace, config.collection)) {
                        false
                    } else {
                        ensureCollection(sdb, config.collectionSpace, config.collection, Option(config))
                        true
                    }
                case _ => false
            }
        } finally {
            sdb.disconnect()
        }
    }

    private def isCollectionExist(sdb: Sequoiadb, csName: String, clName: String): Boolean = {
        if (sdb.isCollectionSpaceExist(csName)) {
            val cs = sdb.getCollectionSpace(csName)
            if (cs.isCollectionExist(clName)) {
                return true
            }
        }

        false
    }

    // create CollectionSpace & Collection if they are not existing
    private def ensureCollection(sdb: Sequoiadb, csName: String, clName: String, config: Option[SdbConfig] = None): DBCollection = {
        if (!sdb.isCollectionSpaceExist(csName)) {
            val options = new BasicBSONObject()
            if (config.nonEmpty) {
                options.put("PageSize", config.get.pageSize)
                options.put("LobPageSize", config.get.lobPageSize)
                val domain = config.get.domain
                if (domain != "") {
                    options.put("Domain", domain)
                }
                logInfo(s"Using $options to create collection space[$csName]")
            }
            sdb.createCollectionSpace(csName, options)
        }
        val cs = sdb.getCollectionSpace(csName)
        if (!cs.isCollectionExist(clName)) {
            val options = new BasicBSONObject()
            if (config.nonEmpty) {
                if (config.get.shardingKey != "") {
                    val shardingKey = JSON.parse(config.get.shardingKey)
                        .asInstanceOf[BSONObject]
                    options.put("ShardingKey", shardingKey)
                    options.put("ShardingType", config.get.shardingType)
                    if (config.get.shardingType == SdbConfig.SHARDING_TYPE_HASH) {
                        options.put("Partition", config.get.clPartition)
                        if (config.get.autoSplit) {
                            options.put("AutoSplit", config.get.autoSplit)
                        }
                    }
                }
                options.put("ReplSize", config.get.replicaSize)
                if (config.get.compressionType != SdbConfig.COMPRESSION_TYPE_NONE) {
                    options.put("Compressed", true)
                    options.put("CompressionType", config.get.compressionType)
                }
                if (config.get.group != "") {
                    options.put("Group", config.get.group)
                }

                if (config.get.ensureShardingIndex != SdbConfig.DefaultEnsureShardingIndex) {
                    options.put("EnsureShardingIndex", config.get.ensureShardingIndex)
                }

                if (config.get.autoIndexId != SdbConfig.DefaultAutoIndexId) {
                    options.put("AutoIndexId", config.get.autoIndexId)
                }
                // auto increment key
                if (config.get.autoIncrement != "") {
                    val autoIncrement = JSON.parse(config.get.autoIncrement)
                        .asInstanceOf[BSONObject]
                    options.put("AutoIncrement", autoIncrement)
                }

                if (config.get.strictDataMode != SdbConfig.DefaultStrictDataMode) {
                    options.put("StrictDataMode", config.get.strictDataMode)
                }

                logInfo(s"Using $options to create collection[$csName.$clName]")
            }
            cs.createCollection(clName, options)
        }
        cs.getCollection(clName)
    }
}
