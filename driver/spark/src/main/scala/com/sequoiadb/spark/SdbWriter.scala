package com.sequoiadb.spark

import java.util

import com.sequoiadb.base.{DBCollection, Sequoiadb}
import org.apache.spark.sql.Row
import org.apache.spark.sql.types.StructType
import org.bson.BSONObject

import scala.collection.JavaConversions._

/**
  * SequoiaDB writer.
  * Used for saving a bunch of sequoiadb objects
  * into specified collectionspace and collection
  *
  * @param config Configuration parameters (host,collectionspace,collection,...)
  */
private[spark] class SdbWriter(config: SdbConfig, sourceInfo: String) extends Serializable with Logging {

    /**
      * Storing a bunch of SequoiaDB objects.
      *
      * @param it Iterator of SequoiaDB objects.
      */
    private def write[T](it: Iterator[T], convert: T => BSONObject): Unit = {
        val sdb = new Sequoiadb(config.host, config.username, config.password, SdbConfig.SdbConnectionOptions)

        SdbConnUtil.setupSourceSessionAttrIgnoreFailures(sdb, sourceInfo)

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

            val flag: Int = if (config.ignoreDuplicateKey) {
                DBCollection.FLG_INSERT_CONTONDUP
            } else {
                0
            }

            val bulk = new util.ArrayList[BSONObject](config.bulkSize)
            var count: Long = 0

            while (it.hasNext) {
                val row = it.next()
                val obj = convert(row)
                if (obj != null) {
                    bulk.add(obj)
                    count += 1
                }

                if (bulk.size() > config.bulkSize) {
                    cl.bulkInsert(bulk, flag)
                    bulk.clear()
                }
            }

            if (bulk.size() > 0) {
                cl.bulkInsert(bulk, flag)
                bulk.clear()
            }

            logInfo(s"write $count records to ${config.collectionSpace}.${config.collection}")
        } finally {
            sdb.disconnect()
        }
    }

    def write(it: Iterator[Row], schema: StructType): Unit = {
        logInfo(s"begin to write rows to ${config.collectionSpace}.${config.collection}," +
            s"${SdbConfig.IgnoreNullField}=${config.ignoreNullField}," +
            s"${SdbConfig.IgnoreDuplicateKey}=${config.ignoreDuplicateKey}")
        write(it, (v: Row) => {
            val obj = BSONConverter.rowToBson(v, schema)
            if (config.ignoreNullField) {
                BSONConverter.removeNullFields(obj)
            } else {
                obj
            }
        })
        logInfo(s"finish writing rows to ${config.collectionSpace}.${config.collection}")
    }

    def write(it: Iterator[BSONObject]): Unit = {
        logInfo(s"begin to write BSONObject to ${config.collectionSpace}.${config.collection}," +
            s"${SdbConfig.IgnoreNullField}=${config.ignoreNullField}," +
            s"${SdbConfig.IgnoreDuplicateKey}=${config.ignoreDuplicateKey}")
        write(it, (v: BSONObject) => {
            if (config.ignoreNullField) {
                BSONConverter.removeNullFields(v)
            } else {
                v
            }
        })
        logInfo(s"finish writing rows to ${config.collectionSpace}.${config.collection}")
    }
}
