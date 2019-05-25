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

import com.sequoiadb.base.{DBCursor, Sequoiadb}
import com.sequoiadb.exception.BaseException
import org.bson.BSONObject
import org.bson.types.BasicBSONList

import scala.collection.JavaConversions._
import scala.collection.mutable
import scala.collection.mutable.ArrayBuffer
import scala.util.control.Breaks

/**
  * SequoiaDB partitioner
  *
  * @param config configurations
  * @param filter query filter
  */
abstract class SdbPartitioner(config: SdbConfig, filter: SdbFilter)
    extends Serializable with Logging {

    def computePartitions(): Array[SdbPartition]
}

private object SdbPartitioner extends Logging {
    def apply(config: SdbConfig, filter: SdbFilter): SdbPartitioner = {
        val mode = determinatePartitionMode(config, filter)
        logInfo(s"PartitionMode=$mode")
        mode match {
            case PartitionMode.Single =>
                new SdbSinglePartitioner(config, filter)
            case PartitionMode.Sharding =>
                new SdbShardingPartitioner(config, filter)
            case PartitionMode.Datablock =>
                new SdbDatablockPartitioner(config, filter)
            case _ =>
                throw new SdbException("Unknown partition mode: " + mode)
        }
    }

    private def determinatePartitionMode(config: SdbConfig, filter: SdbFilter): PartitionMode = {
        logInfo(s"config.partitionMode=${config.partitionMode}")
        config.partitionMode.toLowerCase match {
            case SdbConfig.PARTITION_MODE_SINGLE =>
                if (isDataScheduler(config)) {
                    logInfo(s"Host[${config.host}] is data scheduler, so use Sharding mode instead")
                    PartitionMode.Sharding
                } else {
                    PartitionMode.Single
                }
            case SdbConfig.PARTITION_MODE_SHARDING => PartitionMode.Sharding
            case SdbConfig.PARTITION_MODE_DATABLOCK => PartitionMode.Datablock
            case SdbConfig.PARTITION_MODE_AUTO => autodetectPartitionMode(config, filter)
            case _ =>
                throw new SdbException("Unknown partition mode: " + config.partitionMode)
        }
    }

    private def isDataScheduler(config: SdbConfig): Boolean = {
        val sdb = new Sequoiadb(config.host, config.username, config.password, SdbConfig.SdbConnectionOptions)
        try {
            val cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_DATABASE,
                null.asInstanceOf[BSONObject], null, null)
            while (cursor.hasNext) {
                val obj = cursor.getNext
                if (obj.containsField("Role")) {
                    val role = obj.get("Role").asInstanceOf[String]
                    if (role.equals("DataScheduler")) {
                        return true
                    }
                }
            }
            cursor.close()
        } finally {
            sdb.disconnect()
        }

        false
    }

    // Sharding if query can use index, otherwise Datablock
    private def autodetectPartitionMode(config: SdbConfig, filter: SdbFilter): PartitionMode = {
        val sdb = new Sequoiadb(config.host, config.username, config.password, SdbConfig.SdbConnectionOptions)
        try {
            val shardings = getShardingInfo(sdb, config, filter)
            for (sharding <- shardings) {
                if (sharding.scanType == "ixscan") {
                    return PartitionMode.Sharding
                }
            }

            // partition num limit is too small, use Sharding mode
            if (config.partitionMaxNum > 0 && config.partitionMaxNum <= shardings.length) {
                return PartitionMode.Sharding
            }
        } finally {
            sdb.disconnect()
        }

        PartitionMode.Datablock
    }

    def getAbnormalNodes(sdb: Sequoiadb): Map[String, String] = {
        val map = new mutable.HashMap[String, String]()

        try {
            var cursor: DBCursor = null
            try {
                cursor = sdb.exec("select NodeName, ServiceStatus, Status from $SNAPSHOT_SYSTEM")
            } catch {
                case e: BaseException =>
                    if (e.getErrorType.equals("SDB_INVALIDARG")) {
                        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SYSTEM,
                            null.asInstanceOf[BSONObject], null, null)
                    } else {
                        throw e
                    }
                case x: Throwable => throw x
            }

            while (cursor.hasNext) {
                val obj = cursor.getNext

                if (obj.containsField("ErrNodes")) {
                    val list = obj.get("ErrNodes").asInstanceOf[BasicBSONList]
                    list.foreach { value =>
                        val node = value.asInstanceOf[BSONObject]
                        val nodeName = node.get("NodeName").asInstanceOf[String]
                        val flag = node.get("Flag").asInstanceOf[Int].toString
                        map += (nodeName -> flag)
                    }
                } else if (obj.containsField("ServiceStatus")) {
                    val node = obj
                    val serviceStatus = node.get("ServiceStatus").asInstanceOf[Boolean]
                    if (!serviceStatus) {
                        val nodeName = node.get("NodeName").asInstanceOf[String]
                        val status = node.get("Status").asInstanceOf[String]
                        map += (nodeName -> status)
                    }
                }
            }
            cursor.close()
        } catch {
            case e: BaseException =>
                if (!e.getErrorType.equals("SDB_RTN_COORD_ONLY")) {
                    throw e
                }
            case x: Throwable => throw x
        }

        map.toMap
    }

    def getReplicaGroups(sdb: Sequoiadb): Map[String, List[String]] = {
        val map = new mutable.HashMap[String, List[String]]()

        val abnormalNodes = getAbnormalNodes(sdb)
        if (abnormalNodes.nonEmpty) {
            logInfo(s"Abnormal nodes: $abnormalNodes")
        }

        try {
            val cursor = sdb.listReplicaGroups()
            while (cursor.hasNext) {
                val obj = cursor.getNext

                val groupName = obj.get("GroupName").asInstanceOf[String]
                val nodeList = ArrayBuffer[String]()

                val group = obj.get("Group").asInstanceOf[BasicBSONList]
                for (node <- group) {
                    val nodeObj = node.asInstanceOf[BSONObject]

                    val hostName = nodeObj.get("HostName")
                    val service = nodeObj.get("Service").asInstanceOf[BasicBSONList]
                    for (port <- service) {
                        val portObj = port.asInstanceOf[BSONObject]
                        val serviceType = portObj.get("Type").asInstanceOf[Int]
                        if (serviceType == 0) {
                            val serviceName = portObj.get("Name").asInstanceOf[String]

                            val node = hostName + ":" + serviceName

                            if (abnormalNodes.isEmpty || !abnormalNodes.contains(node)) {
                                nodeList += node
                            }
                        }
                    }
                }

                map += (groupName -> nodeList.sorted.toList)
            }
            cursor.close()
        } catch {
            case e: BaseException =>
                if (!e.getErrorType.equals("SDB_RTN_COORD_ONLY")) {
                    throw e
                }
            case x: Throwable => throw x
        }

        map.toMap
    }

    def getShardingInfo(sdb: Sequoiadb, config: SdbConfig, filter: SdbFilter): List[ShardingInfo] = {
        val shardingList = ArrayBuffer[ShardingInfo]()

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

        val matcher = filter.BSONObj()

        val cursor = cl.explain(matcher, null, null, null, 0, -1, 0, null)
        if (cursor == null) {
            return List[ShardingInfo]()
        }

        while (cursor.hasNext) {
            val obj = cursor.getNext

            val nodeName = obj.get("NodeName").asInstanceOf[String]
            val groupName = obj.get("GroupName").asInstanceOf[String]

            if (obj.containsField("SubCollections")) {
                val subCollections = obj.get("SubCollections").asInstanceOf[BasicBSONList]
                for (sub <- subCollections) {
                    val subObj = sub.asInstanceOf[BSONObject]

                    val name = subObj.get("Name").asInstanceOf[String]
                    val scanType = subObj.get("ScanType").asInstanceOf[String]

                    val shardingInfo = new ShardingInfo(name, nodeName, groupName, scanType)
                    shardingList += shardingInfo
                }
            } else {
                val name = obj.get("Name").asInstanceOf[String]
                val scanType = obj.get("ScanType").asInstanceOf[String]

                val shardingInfo = new ShardingInfo(name, nodeName, groupName, scanType)
                shardingList += shardingInfo
            }
        }

        shardingList.toList
    }

    def getQueryMeta(url: String, csName: String, clName: String, config: SdbConfig): List[QueryMeta] = {
        val queryMetas = ArrayBuffer[QueryMeta]()

        val sdb = new Sequoiadb(url, config.username, config.password, SdbConfig.SdbConnectionOptions)
        try {
            val cs = if (sdb.isCollectionSpaceExist(csName)) {
                sdb.getCollectionSpace(csName)
            } else {
                throw new SdbException(s"Collection space is not existing: $csName")
            }

            val cl = if (cs.isCollectionExist(clName)) {
                cs.getCollection(clName)
            } else {
                throw new SdbException(s"Collection is not existing: " +
                    s"$csName.$clName")
            }

            val cursor = cl.getQueryMeta(null, null, null, 0, -1, 0)
            while (cursor.hasNext) {
                val obj = cursor.getNext

                val hostName = obj.get("HostName").asInstanceOf[String]
                val serviceName = obj.get("ServiceName").asInstanceOf[String]

                val blocks = ArrayBuffer[Int]()

                val datablocks = obj.get("Datablocks").asInstanceOf[BasicBSONList]
                for (blockId <- datablocks) {
                    blocks += blockId.asInstanceOf[Int]
                }

                val meta = new QueryMeta(hostName + ":" + serviceName,
                    csName, clName, blocks.toList)
                queryMetas += meta
            }
        } finally {
            sdb.disconnect()
        }

        queryMetas.toList
    }

    // shuffle partitions to re-arrange partition position
    def shufflePartitions(partitions: Array[SdbPartition]): Array[SdbPartition] = {
        val partitionSelector: PartitionSelector = new PartitionSelector

        // collect which host and node the partition is placed
        partitions.foreach { partition =>
            val url = partition.urls.head
            partitionSelector.addPartition(url, partition)
        }

        val newPartitions = ArrayBuffer[SdbPartition]()

        // chose partition which is in the least used host and node one by one
        while (partitionSelector.hasHosts) {
            val minNode = partitionSelector.getLeastUsedNode

            val partition = minNode.partitions.head
            newPartitions += partition

            partitionSelector.removePartition(minNode, partition)
        }

        if (newPartitions.size != partitions.length) {
            throw new SdbException("shufflePartitions")
        }

        for (i <- newPartitions.indices) {
            newPartitions(i).index = i
        }

        newPartitions.toArray
    }
}

// Host container
private class Host(val host: String) {
    val nodes: ArrayBuffer[Node] = ArrayBuffer[Node]()
    val nodeMap: mutable.HashMap[Int, Node] = mutable.HashMap[Int, Node]()
    var partitionNum: Int = 0
    var refCount: Int = 0

    override def toString: String = host
}

// Node container
private class Node(val host: String, val port: Int) {
    val partitions: ArrayBuffer[SdbPartition] = ArrayBuffer[SdbPartition]()
    var refCount: Int = 0

    override def toString: String = s"$host:$port"
}

private class PartitionSelector {
    private val hostMap = mutable.HashMap[String, Host]()

    def hasHosts: Boolean = hostMap.nonEmpty

    def addPartition(url: String, partition: SdbPartition): Unit = {
        val u = url.split(':')
        val hostName = u(0).trim
        val port = u(1).trim.toInt

        val host = hostMap.get(hostName)
        if (host.isEmpty) {
            val newNode = new Node(hostName, port)
            newNode.partitions += partition

            val newHost = new Host(hostName)
            newHost.nodeMap += (port -> newNode)
            newHost.nodes += newNode
            newHost.partitionNum += 1

            hostMap += (hostName -> newHost)
        } else {
            val oldHost = host.get
            val node = oldHost.nodeMap.get(port)
            if (node.isEmpty) {
                val newNode = new Node(hostName, port)
                newNode.partitions += partition
                oldHost.nodeMap += (port -> newNode)
                oldHost.nodes += newNode
                oldHost.partitionNum += 1
            } else {
                node.get.partitions += partition
                oldHost.partitionNum += 1
            }
        }
    }

    def removePartition(node: Node, partition: SdbPartition): Unit = {
        val host: Host = hostMap.get(node.host).orNull
        if (host == null) {
            throw new SdbException(s"No host for node $node")
        }

        node.partitions -= partition
        node.refCount += 1
        host.refCount += 1
        host.partitionNum -= 1

        if (node.partitions.isEmpty) {
            host.nodes -= node
            if (host.nodes.isEmpty) {
                hostMap.remove(node.host)
            }
        }
    }

    def getLeastUsedNode: Node = {
        var minHost: Host = null

        hostMap.values.foreach { host: Host =>
            if (minHost == null) {
                minHost = host
            } else {
                if (host.refCount < minHost.refCount) {
                    minHost = host
                }
            }
        }

        if (minHost == null) {
            return null
        }

        var minNode: Node = minHost.nodes.head
        minHost.nodes.foreach { node: Node =>
            if (node.refCount < minNode.refCount) {
                minNode = node
            }
        }

        minNode
    }
}

private class NodeSelector {
    private val hostMap = mutable.HashMap[String, Host]()

    private def ensureHost(hostName: String): Unit = {
        val host = hostMap.get(hostName)
        if (host.isEmpty) {
            val newHost = new Host(hostName)
            hostMap += (hostName -> newHost)
        }
    }

    private def ensureNode(hostName: String, port: Int): Unit = {
        ensureHost(hostName)
        val host = hostMap.get(hostName)
        if (host.isEmpty) {
            throw new SdbException(s"No host: [$hostName]")
        }

        val node = host.get.nodeMap.get(port)
        if (node.isEmpty) {
            val newNode = new Node(hostName, port)
            host.get.nodeMap += (port -> newNode)
            host.get.nodes += newNode
        }

        if (host.get.nodeMap.get(port).isEmpty) {
            throw new SdbException(s"No node: [$hostName:$port]")
        }
    }

    def increaseNodeRefCount(url: String): Unit = {
        val u = url.split(':')
        val hostName = u(0).trim
        val port = u(1).trim.toInt

        ensureNode(hostName, port)

        val host = hostMap(hostName)
        val node = host.nodeMap(port)

        host.refCount += 1
        node.refCount += 1
    }

    def getLeastUsedNode(urls: List[String]): String = {
        var leastUsedHost: Host = null
        var leastUsedNode: Node = null
        var leastUsedUrl: String = null

        if (urls == null || urls.isEmpty) {
            throw new SdbException("Null or empty urls")
        }

        urls.foreach { url =>
            val u = url.split(':')
            val hostName = u(0).trim
            val port = u(1).trim.toInt

            ensureNode(hostName, port)

            val host = hostMap(hostName)
            val node = host.nodeMap(port)

            if (leastUsedNode == null) {
                leastUsedHost = host
                leastUsedNode = node
                leastUsedUrl = url
            } else {
                if (host != leastUsedHost) {
                    if (host.refCount < leastUsedHost.refCount) {
                        leastUsedHost = host
                        leastUsedNode = node
                        leastUsedUrl = url
                    }
                } else {
                    if (node.refCount < leastUsedNode.refCount) {
                        leastUsedNode = node
                        leastUsedUrl = url
                    }
                }
            }
        }

        leastUsedUrl
    }
}

/**
  * Sharding info of SequoiaDB collection
  *
  * @param name      sharding collection full name
  * @param nodeName  the url of SequoiaDB node that return the sharding info
  * @param groupName the group that hold the sharding collection
  * @param scanType  the query scan type
  */
private class ShardingInfo(val name: String,
                           val nodeName: String,
                           val groupName: String,
                           val scanType: String) {
    val csName: String = name.split('.')(0)
    val clName: String = name.split('.')(1)
}

/**
  * Query meta data of SequoiaDB
  *
  * @param url        url of SequoiaDB node that hode the datablocks
  * @param datablocks datablocks of partition
  */
private class QueryMeta(val url: String,
                        val csName: String,
                        val clName: String,
                        val datablocks: List[Int]) {
}

// Single partitioner generates only one partition using the host in SdbConfig
private[spark] class SdbSinglePartitioner(config: SdbConfig, filter: SdbFilter)
    extends SdbPartitioner(config, filter) {

    override def computePartitions(): Array[SdbPartition] = {
        Array(SdbPartition(
            config.host,
            config.collectionSpace,
            config.collection,
            filter,
            PartitionMode.Single))
    }
}

// Sharding partitioner generates partitions by SequoiaDB sharding
private[spark] class SdbShardingPartitioner(config: SdbConfig, filter: SdbFilter)
    extends SdbPartitioner(config, filter) {

    override def computePartitions(): Array[SdbPartition] = {
        val partitions = ArrayBuffer[SdbPartition]()

        val sdb = new Sequoiadb(config.host, config.username, config.password, SdbConfig.SdbConnectionOptions)
        try {
            val nodeSelector = new NodeSelector

            val groups = SdbPartitioner.getReplicaGroups(sdb)
            val shardings = SdbPartitioner.getShardingInfo(sdb, config, filter)

            for (sharding <- shardings) {
                var urls: List[String] = null
                if (sharding.groupName == "") {
                    urls = List(sharding.nodeName)
                } else {
                    val groupNodeUrls = groups(sharding.groupName)
                    if (groupNodeUrls.isEmpty) {
                        throw new SdbException(s"Group ${sharding.groupName} has no normal node")
                    }
                    if (config.shardingPartitionSingleNode) {
                        val url = nodeSelector.getLeastUsedNode(groupNodeUrls)
                        nodeSelector.increaseNodeRefCount(url)
                        urls = List(url)
                    } else {
                        urls = groupNodeUrls
                    }
                }

                val partition = SdbPartition(
                    urls,
                    sharding.csName,
                    sharding.clName,
                    filter,
                    PartitionMode.Sharding)
                partitions += partition
            }
        } finally {
            sdb.disconnect()
        }

        SdbPartitioner.shufflePartitions(partitions.toArray)
    }
}

// Datablock partitioner generates partitions by SequoiaDB collection's data blocks
private[spark] class SdbDatablockPartitioner(config: SdbConfig, filter: SdbFilter)
    extends SdbPartitioner(config, filter) {

    override def computePartitions(): Array[SdbPartition] = {
        var groups: Map[String, List[String]] = null
        var shardings: List[ShardingInfo] = null

        val sdb = new Sequoiadb(config.host, config.username, config.password, SdbConfig.SdbConnectionOptions)
        try {
            groups = SdbPartitioner.getReplicaGroups(sdb)
            shardings = SdbPartitioner.getShardingInfo(sdb, config, filter)
        } finally {
            sdb.disconnect()
        }

        val queryMetas = getQueryMetas(groups, shardings)

        var partitions: Array[SdbPartition] = null
        var blockNum = config.partitionBlockNum

        val breaker = new Breaks
        breaker.breakable {
            while (true) {
                partitions = generatePartitions(queryMetas, blockNum)

                // zero means no limit for partition max num
                if (config.partitionMaxNum == 0) {
                    breaker.break()
                }

                // partition num is satisfied or only one partition per sharding
                if (partitions.length < config.partitionMaxNum ||
                    partitions.length == shardings.size) {
                    breaker.break()
                }

                // we should increase block num of each partition to decrease partition num
                blockNum += 1
            }
        }

        logInfo(s"get ${partitions.length} partitions with $blockNum data blocks per partition")

        SdbPartitioner.shufflePartitions(partitions)
    }

    private def getQueryMetas(groups: Map[String, List[String]],
                              shardings: List[ShardingInfo])
    : Array[QueryMeta] = {
        val queryMetas = ArrayBuffer[QueryMeta]()
        val nodeSelector = new NodeSelector

        for (sharding <- shardings) {
            var url: String = null
            if (sharding.groupName == "") {
                url = sharding.nodeName
            } else {
                val groupNodeUrls = groups(sharding.groupName)
                if (groupNodeUrls.isEmpty) {
                    throw new SdbException(s"Group ${sharding.groupName} has no normal node")
                }
                url = nodeSelector.getLeastUsedNode(groupNodeUrls)
                nodeSelector.increaseNodeRefCount(url)
            }

            val metas = SdbPartitioner.getQueryMeta(url, sharding.csName, sharding.clName, config)
            queryMetas ++= metas
        }

        queryMetas.toArray
    }

    private def generatePartitions(queryMetas: Array[QueryMeta],
                                   blockNum: Int)
    : Array[SdbPartition] = {
        val partitions = ArrayBuffer[SdbPartition]()

        for (meta <- queryMetas) {
            for (blocks <- meta.datablocks.sliding(blockNum, blockNum)) {
                val partition = SdbPartition(List(meta.url),
                    meta.csName,
                    meta.clName,
                    filter,
                    PartitionMode.Datablock,
                    blocks)
                partitions += partition
            }
        }

        partitions.toArray
    }
}