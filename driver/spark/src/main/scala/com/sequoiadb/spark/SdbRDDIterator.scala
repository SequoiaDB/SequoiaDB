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

import com.sequoiadb.base.{DBCollection, DBCursor, Sequoiadb}
import org.apache.spark.sql.Row
import org.apache.spark.sql.types.StructType
import org.bson.types.BasicBSONList
import org.bson.{BSONObject, BasicBSONObject}

import scala.collection.JavaConversions._
import scala.reflect.ClassTag

/**
  * SequoiaDB RDD iterator
  *
  * @param config          configurations
  * @param partition       the partitions of RDD
  * @param requiredColumns query selector
  * @param numReturned     query returned num
  */
abstract class SdbRDDIterator[T: ClassTag](config: SdbConfig,
                                           sourceInfo: String,
                                           partition: SdbPartition,
                                           requiredColumns: Array[String],
                                           numReturned: Long = -1)
    extends Iterator[T] with Logging with java.io.Closeable with Serializable {

    // chose the url of current host if it exists
    private def preferredLocalHost(urls: List[String]): List[String] = {
        val localHost = InetAddress.getLocalHost
        for (url <- urls) {
            val host = url.split(':')(0).trim
            val address = InetAddress.getByName(host)
            if (address == localHost) {
                return List(url)
            }
        }

        urls
    }

    private val url: List[String] = {
        if (partition.urls.size > 1 && config.preferredLocation) {
            preferredLocalHost(partition.urls)
        } else {
            partition.urls
        }
    }

    private val sdb = {
        val conn = new Sequoiadb(
            url,
            config.username,
            config.password,
            SdbConfig.SdbConnectionOptions)

        SdbConnUtil.setupSourceSessionAttrIgnoreFailures(conn, sourceInfo)
        conn
    }

    // build query hint for datablock partition
    private val hint: BSONObject = {
        val hintObj = new BasicBSONObject()

        if (partition.mode == PartitionMode.Datablock) {
            val datablocks = new BasicBSONList()
            partition.blocks.foreach(blockId =>
                datablocks.add(blockId.asInstanceOf[Integer]))

            val metaObj = new BasicBSONObject()
            metaObj.put("Datablocks", datablocks)
            metaObj.put("ScanType", "tbscan")

            hintObj.put("$Meta", metaObj)
        }

        hintObj
    }

    private val selector: BSONObject = SdbRDDIterator.createSelector(requiredColumns)

    logInfo(s"selector=$selector")

    private val cl: DBCollection = {
        val cs = if (sdb.isCollectionSpaceExist(partition.csName)) {
            sdb.getCollectionSpace(partition.csName)
        } else {
            throw new SdbException(s"Collection space is not existing: ${partition.csName}")
        }

        val cl = if (cs.isCollectionExist(partition.clName)) {
            cs.getCollection(partition.clName)
        } else {
            throw new SdbException(s"Collection is not existing: " +
                s"${partition.csName}.${partition.clName}")
        }

        cl
    }

    private val cursor: DBCursor = cl.query(
        partition.filter.BSONObj(),
        selector,
        null, hint, 0, numReturned, 0)

    protected lazy val sdbCursor: SdbCursor = {
        if (config.cursorType.equalsIgnoreCase(SdbConfig.CURSOR_TYPE_FAST)) {
            logInfo(s"create fast cursor, " +
                s"buf size = ${config.fastCursorBufSize}, " +
                s"decoder num = ${config.fastCursorDecoderNum}")
            new SdbFastCursor(cursor, config.fastCursorBufSize, config.fastCursorDecoderNum)
        } else {
            logInfo("create normal cursor")
            new SdbNormalCursor(cursor)
        }
    }

    override def hasNext: Boolean = {
        sdbCursor.hasNext
    }

    override def next(): T

    def close(): Unit = {
        if (!sdb.isClosed) {
            sdbCursor.close()
            sdb.disconnect()
        }
    }
}

object SdbRDDIterator {
    private def createSelector(requiredColumns: Array[String]): BSONObject = {
        val selector: BSONObject = new BasicBSONObject
        requiredColumns.map {
            selector.put(_, null)
        }
        selector
    }
}

/**
  * SequoiaDB RDD[Row] iterator
  *
  * @param sdbConfig       configurations
  * @param partition       the partitions of RDD
  * @param schema          the schema of Row
  * @param requiredColumns query selector
  */
class SdbRowRDDIterator(sdbConfig: SdbConfig,
                        sourceInfo: String,
                        partition: SdbPartition,
                        schema: StructType,
                        requiredColumns: Array[String])
    extends SdbRDDIterator[Row](sdbConfig, sourceInfo, partition, requiredColumns) {

    override def next(): Row = {
        val obj = sdbCursor.next()
        BSONConverter.bsonToRow(obj, schema)
    }
}

/**
  * SequoiaDB RDD[BSONObject] iterator
  *
  * @param sdbConfig       configurations
  * @param partition       the partitions of RDD
  * @param requiredColumns query selector
  * @param numReturned     query returned num
  */
class SdbBsonRDDIterator(sdbConfig: SdbConfig,
                         sourceInfo: String,
                         partition: SdbPartition,
                         requiredColumns: Array[String],
                         numReturned: Long = -1)
    extends SdbRDDIterator[BSONObject](sdbConfig, sourceInfo, partition, requiredColumns, numReturned) {

    override def next(): BSONObject = sdbCursor.next()
}