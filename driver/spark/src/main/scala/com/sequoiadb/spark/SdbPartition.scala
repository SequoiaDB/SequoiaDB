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

import org.apache.spark.Partition

/**
  * A SequoiaDB Partition is a minimum unit of repeatable-read operation
  * In the current release each SequoiadbPartition represent a shard
  * in the cluster, that means whenever a partition operation failed during the middle
  * it's always safe to restart from scratch on a different node
  *
  * @param urls   urls of SequoiaDB nodes that hold partition data
  * @param csName collection space name
  * @param clName collection name
  * @param filter partition query filter
  * @param mode   PartitionMode
  * @param blocks datablocks for PartitionMode.Datablock
  */
class SdbPartition(val urls: List[String],
                   val csName: String,
                   val clName: String,
                   val filter: SdbFilter,
                   val mode: PartitionMode,
                   val blocks: List[Int] = List())
    extends Partition {

    var index: Int = 0

    override def toString: String = {
        s"SdbPartition{index: $index, url: $urls, cl: $csName.$clName, " +
            s"filter: $filter, mode: $mode, blocks: $blocks}"
    }
}

object SdbPartition {
    def apply(urls: List[String],
              csName: String,
              clName: String,
              filter: SdbFilter,
              mode: PartitionMode,
              blocks: List[Int] = List()): SdbPartition =
        new SdbPartition(urls, csName, clName, filter, mode, blocks)
}
