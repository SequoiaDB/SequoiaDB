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

import com.sequoiadb.net.ConfigOptions
import org.bson.BSONObject
import org.bson.util.JSON

/**
  * SequoiaDB configurations
  *
  * @param properties configurations in Map
  */
class SdbConfig(val properties: Map[String, String]) extends Serializable {
    private def notFound(name: String): Nothing = {
        throw new SdbException(s"Parameter $name is not specified")
    }

    private def invalidConfigValue(name: String, value: Any): Nothing = {
        throw new SdbException(s"Invalid value of parameter $name: $value")
    }

    require(
        SdbConfig.RequiredProperties.forall(properties.isDefinedAt),
        s"Not all required properties are defined! : ${
            SdbConfig.RequiredProperties.diff(
                properties.keys.toList.intersect(SdbConfig.RequiredProperties))
        }")

    // don't show password
    override def toString: String = (properties -- Seq(SdbConfig.Password)).toString()

    val host: List[String] = properties
        .getOrElse(SdbConfig.Host, notFound(SdbConfig.Host))
        .split(",").toList

    val collectionSpace: String = properties
        .getOrElse(SdbConfig.CollectionSpace, notFound(SdbConfig.CollectionSpace))

    val collection: String = properties
        .getOrElse(SdbConfig.Collection, notFound(SdbConfig.Collection))

    val username: String = properties
        .getOrElse(SdbConfig.Username, SdbConfig.DefaultUsername)

    val password: String = properties
        .getOrElse(SdbConfig.Password, SdbConfig.DefaultPassword)

    val samplingRatio: Double = properties.get(SdbConfig.SamplingRatio)
        .map(_.toDouble).getOrElse(SdbConfig.DefaultSamplingRatio)

    if (samplingRatio <= 0 || samplingRatio > 1.0) {
        invalidConfigValue(SdbConfig.SamplingRatio, samplingRatio)
    }

    val samplingNum: Long = properties.get(SdbConfig.SamplingNum)
        .map(_.toLong).getOrElse(SdbConfig.DefaultSamplingNum)

    if (samplingNum <= 0) {
        invalidConfigValue(SdbConfig.SamplingNum, samplingNum)
    }

    /**
      * sample _id for schema if true, ignore _id if false.
      */
    val samplingWithId: Boolean = properties.get(SdbConfig.SamplingWithId)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultSamplingWithId)

    /**
      * use single partition mode for schema sampling if true, use partitionMode if false.
      */
    val samplingSingle: Boolean = properties.get(SdbConfig.SamplingSingle)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultSamplingSingle)

    /**
      * bulk size when insert into SequoiaDB collection
      */
    val bulkSize: Int = properties.get(SdbConfig.BulkSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultBulkSize)

    if (bulkSize <= 0) {
        invalidConfigValue(SdbConfig.BulkSize, bulkSize)
    }

    val cursorType: String = properties
        .getOrElse(SdbConfig.CursorType, SdbConfig.DefaultCursorType)

    cursorType.toLowerCase match {
        case SdbConfig.CURSOR_TYPE_FAST =>
        case SdbConfig.CURSOR_TYPE_NORMAL =>
        case _ =>
            invalidConfigValue(SdbConfig.CursorType, cursorType)
    }

    val fastCursorBufSize: Int = properties.get(SdbConfig.FastCursorBufSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultFastCursorBufSize)

    if (fastCursorBufSize <= 0) {
        invalidConfigValue(SdbConfig.FastCursorBufSize, fastCursorBufSize)
    }

    val fastCursorDecoderNum: Int = properties.get(SdbConfig.FastCursorDecoderNum)
        .map(_.toInt).getOrElse(SdbConfig.DefaultFastCursorDecoderNum)

    if (fastCursorDecoderNum <= 0 || fastCursorDecoderNum > 16) {
        invalidConfigValue(SdbConfig.FastCursorDecoderNum, fastCursorDecoderNum)
    }

    private val scanType: String = properties
        .getOrElse(SdbConfig.ScanType, SdbConfig.SCAN_TYPE_AUTO)

    val partitionMode: String = properties
        .getOrElse(SdbConfig.PartitionMode, {
            // compatible with old edition option
            scanType.toLowerCase match {
                case SdbConfig.SCAN_TYPE_IXSCAN =>
                    SdbConfig.PARTITION_MODE_SHARDING
                case SdbConfig.SCAN_TYPE_TBSCAN =>
                    SdbConfig.PARTITION_MODE_DATABLOCK
                case _ => SdbConfig.DefaultPartitionMode
            }
        })

    partitionMode.toLowerCase match {
        case SdbConfig.PARTITION_MODE_SINGLE =>
        case SdbConfig.PARTITION_MODE_SHARDING =>
        case SdbConfig.PARTITION_MODE_DATABLOCK =>
        case SdbConfig.PARTITION_MODE_AUTO =>
        case _ =>
            invalidConfigValue(SdbConfig.PartitionMode, partitionMode)
    }

    val partitionBlockNum: Int = properties.get(SdbConfig.PartitionBlockNum)
        .map(_.toInt).getOrElse(SdbConfig.DefaultPartitionBlockNum)

    if (partitionBlockNum <= 0) {
        invalidConfigValue(SdbConfig.PartitionBlockNum, partitionBlockNum)
    }

    val partitionMaxNum: Int = properties.get(SdbConfig.PartitionMaxNum)
        .map(_.toInt).getOrElse(SdbConfig.DefaultPartitionMaxNum)

    // zero means no limit for partition max num when partitionMode is Datablock
    if (partitionMaxNum < 0) {
        invalidConfigValue(SdbConfig.PartitionMaxNum, partitionMaxNum)
    }

    val preferredLocation: Boolean = properties.get(SdbConfig.PreferredLocation)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultPreferredLocation)

    val shardingPartitionSingleNode: Boolean = properties.get(SdbConfig.ShardingPartitionSingleNode)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultShardingPartitionSingleNode)

    val ignoreDuplicateKey: Boolean = properties.get(SdbConfig.IgnoreDuplicateKey)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultIgnoreDuplicateKey)

    val useSelector: String = properties
        .getOrElse(SdbConfig.UseSelector, SdbConfig.DefaultUseSelector)

    useSelector.toLowerCase match {
        case SdbConfig.USE_SELECTOR_ENABLE =>
        case SdbConfig.USE_SELECTOR_DISABLE =>
        case SdbConfig.USE_SELECTOR_AUTO =>
        case _ =>
            invalidConfigValue(SdbConfig.UseSelector, useSelector)
    }

    val selectorDiff: Int = properties.get(SdbConfig.SelectorDiff)
        .map(_.toInt).getOrElse(SdbConfig.DefaultSelectorDiff)

    if (selectorDiff < 0) {
        invalidConfigValue(SdbConfig.SelectorDiff, selectorDiff)
    }

    val pageSize: Int = properties.get(SdbConfig.PageSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultPageSize)

    if (pageSize < 0) {
        invalidConfigValue(SdbConfig.PageSize, pageSize)
    }

    val lobPageSize: Int = properties.get(SdbConfig.LobPageSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultLobPageSize)

    if (lobPageSize < 0) {
        invalidConfigValue(SdbConfig.LobPageSize, lobPageSize)
    }

    val domain: String = properties
        .getOrElse(SdbConfig.Domain, SdbConfig.DefaultDomain)

    val shardingKey: String = properties
        .getOrElse(SdbConfig.ShardingKey, SdbConfig.DefaultShardingKey)

    try {
        JSON.parse(shardingKey).asInstanceOf[BSONObject]
    } catch {
        case _: Exception => invalidConfigValue(SdbConfig.ShardingKey, shardingKey)
    }

    val shardingType: String = properties
        .getOrElse(SdbConfig.ShardingType, SdbConfig.DefaultShardingType)

    shardingType.toLowerCase match {
        case SdbConfig.SHARDING_TYPE_HASH =>
        case SdbConfig.SHARDING_TYPE_RANGE =>
        case SdbConfig.SHARDING_TYPE_NONE =>
            if (shardingKey != "") {
                invalidConfigValue(SdbConfig.ShardingType, shardingType)
            }
        case _ =>
            invalidConfigValue(SdbConfig.ShardingType, shardingType)
    }

    val clPartition: Int = properties.get(SdbConfig.CLPartition)
        .map(_.toInt).getOrElse(SdbConfig.DefaultCLPartition)

    if (clPartition < 0) {
        invalidConfigValue(SdbConfig.CLPartition, clPartition)
    }

    val replicaSize: Int = properties.get(SdbConfig.ReplicaSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultReplicaSize)

    val compressionType: String = properties
        .getOrElse(SdbConfig.CompressionType, SdbConfig.DefaultCompressionType)

    compressionType.toLowerCase match {
        case SdbConfig.COMPRESSION_TYPE_SNAPPY =>
        case SdbConfig.COMPRESSION_TYPE_LZW =>
        case SdbConfig.COMPRESSION_TYPE_NONE =>
        case _ =>
            invalidConfigValue(SdbConfig.CompressionType, compressionType)
    }

    val autoSplit: Boolean = properties.get(SdbConfig.AutoSplit)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultAutoSplit)

    val group: String = properties
        .getOrElse(SdbConfig.Group, SdbConfig.DefaultGroup)
}

object SdbConfig {
    //  Parameter names
    val Host = "host"
    val CollectionSpace = "collectionspace"
    val Collection = "collection"
    val Username = "username"
    val Password = "password"
    val SamplingRatio = "samplingratio"
    val SamplingNum = "samplingnum"
    val SamplingWithId = "samplingwithid"
    val SamplingSingle = "samplingsingle"
    val BulkSize = "bulksize"
    val CursorType = "cursortype"
    // fast, normal
    val FastCursorBufSize = "fastcursorbufsize"
    val FastCursorDecoderNum = "fastcursordecodernum"
    val PartitionMode = "partitionmode"
    // single, sharding, datablock, auto
    val PartitionBlockNum = "partitionblocknum"
    val PartitionMaxNum = "partitionmaxnum"
    val ShardingPartitionSingleNode = "shardingpartitionsinglenode"
    val PreferredLocation = "preferredlocation"
    val IgnoreDuplicateKey = "ignoreduplicatekey"
    val UseSelector = "useselector"
    val SelectorDiff = "selectordiff"
    val PageSize = "pagesize"
    val LobPageSize = "lobpagesize"
    val Domain = "domain"
    val ShardingKey = "shardingkey"
    val ShardingType = "shardingtype"
    val CLPartition = "clpartition"
    val ReplicaSize = "replsize"
    val CompressionType = "compressiontype"
    val AutoSplit = "autosplit"
    val Group = "group"

    // compatible with old edition option
    val ScanType = "scantype" // auto/ixscan/tbscan

    val CURSOR_TYPE_FAST = "fast"
    val CURSOR_TYPE_NORMAL = "normal"

    val PARTITION_MODE_SINGLE = "single"
    val PARTITION_MODE_SHARDING = "sharding"
    val PARTITION_MODE_DATABLOCK = "datablock"
    val PARTITION_MODE_AUTO = "auto"

    val SCAN_TYPE_IXSCAN = "ixscan"
    val SCAN_TYPE_TBSCAN = "tbscan"
    val SCAN_TYPE_AUTO = "auto"

    val USE_SELECTOR_ENABLE = "enable"
    val USE_SELECTOR_DISABLE = "disable"
    val USE_SELECTOR_AUTO = "auto"

    val SHARDING_TYPE_NONE = "none"
    val SHARDING_TYPE_HASH = "hash"
    val SHARDING_TYPE_RANGE = "range"

    val COMPRESSION_TYPE_NONE = "none"
    val COMPRESSION_TYPE_SNAPPY = "snappy"
    val COMPRESSION_TYPE_LZW = "lzw"

    val AllProperties = List(
        Host,
        CollectionSpace,
        Collection,
        Username,
        Password,
        SamplingRatio,
        SamplingNum,
        SamplingWithId,
        SamplingSingle,
        BulkSize,
        CursorType,
        FastCursorBufSize,
        FastCursorDecoderNum,
        PartitionMode,
        PartitionBlockNum,
        PartitionMaxNum,
        ShardingPartitionSingleNode,
        PreferredLocation,
        IgnoreDuplicateKey,
        UseSelector,
        SelectorDiff,
        ScanType,
        PageSize,
        LobPageSize,
        Domain,
        ShardingKey,
        ShardingType,
        CLPartition,
        ReplicaSize,
        CompressionType,
        AutoSplit,
        Group
    )

    val RequiredProperties = List(
        Host,
        CollectionSpace,
        Collection
    )

    //  Default values
    val DefaultUsername = ""
    val DefaultPassword = ""
    val DefaultSamplingRatio = 1.0
    val DefaultSamplingNum = 1000L
    val DefaultSamplingWithId = false
    val DefaultSamplingSingle = true
    val DefaultBulkSize = 500
    val DefaultCursorType = CURSOR_TYPE_FAST
    val DefaultFastCursorBufSize = 500
    val DefaultFastCursorDecoderNum = 2
    val DefaultPartitionMode = PARTITION_MODE_AUTO
    val DefaultPartitionBlockNum = 4
    val DefaultPartitionMaxNum = 1000
    val DefaultShardingPartitionSingleNode = false
    val DefaultPreferredLocation = false
    val DefaultIgnoreDuplicateKey = false
    val DefaultUseSelector = USE_SELECTOR_ENABLE
    val DefaultSelectorDiff = 2
    val DefaultPageSize = 1024 * 64
    val DefaultLobPageSize = 1024 * 256
    val DefaultDomain = ""
    val DefaultShardingKey = ""
    val DefaultShardingType = SHARDING_TYPE_HASH
    val DefaultCLPartition = 1024
    val DefaultReplicaSize = 1
    val DefaultCompressionType = COMPRESSION_TYPE_NONE
    val DefaultAutoSplit = false
    val DefaultGroup = ""

    def apply(parameters: Map[String, String]): SdbConfig = new SdbConfig(parameters)

    private[spark] val SdbConnectionOptions: ConfigOptions = {
        val opt = new ConfigOptions()
        opt.setConnectTimeout(3000)
        opt.setMaxAutoConnectRetryTime(0)
        opt
    }
}
