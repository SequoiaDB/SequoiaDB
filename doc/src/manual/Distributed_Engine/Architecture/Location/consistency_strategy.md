[^_^]:
    同步一致性

同步一致性策略用于控制复制组内优先同步的节点，实现数据同步的精细化管理。SequoiaDB 巨杉数据库基于[节点一致性][node_consistency]策略和位置集，提供不同级别的同步一致性策略，以适配不同的业务场景。下述以 [ReplSize][ReplSize] 取值为 2、存在三个位置集为例，介绍不同级别的同步一致性策略。

##节点优先策略##

节点优先策略适用于对性能要求高的业务场景。在该策略下，数据同步将依据 ReplSize 的取值，随机选取复制组中的节点进行数据同步。同步示意图如下：

![节点优先示意图][consistencystrategy1]

集合的 [ConsistencyStrategy][ConsistencyStrategy] 取值为 1 时，表示按节点优先进行数据同步。当已同步节点数达到 ReplSize 指定的值时，服务端即可返回应答给客户端。

##位置多数派优先策略##

位置多数派优先策略适用于对数据安全要求高的业务场景。在该策略下，数据同步将依据 ReplSize 的取值，优先保证多数派位置集中均存在数据一致的节点。如果满足上述要求后仍未达到 ReplSize 指定的节点数，将同步其他位置集内的节点。同步示意图如下：

![位置多数派优先示意图][consistencystrategy2]

集合的 ConsistencyStrategy 取值为 2 时，表示按位置多数派优先进行数据同步。当主位置集整体故障时，能够保证其他位置集仍存在可用的完整数据，避免数据丢失。

##主位置多数派优先策略##

主位置多数派优先策略适用于对网络延时要求高的业务场景。在该策略下，数据同步将依据 ReplSize 的取值，优先保证主位置集中多数派节点数据一致。如果满足上述要求后仍未达到 ReplSize 指定的节点数，将同步其他位置集内的节点。同步示意图如下：

![主位置多数派优先示意图][consistencystrategy3]

集合的 ConsistencyStrategy 取值为 3 时，表示按主位置多数派优先进行数据同步。当主节点发生故障时，能够保证在主位置集内重新选主，避免因主节点切换至其他位置集而导致的高网络延时。

[^_^]:
    本文使用的所有引用及链接
[consistencystrategy1]:images/Distributed_Engine/Architecture/Location/consistencystrategy1.png
[consistencystrategy2]:images/Distributed_Engine/Architecture/Location/consistencystrategy2.png
[consistencystrategy3]:images/Distributed_Engine/Architecture/Location/consistencystrategy3.png
[session_attr]: manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md
[node_consistency]:manual/Distributed_Engine/Architecture/Replication/primary_secondary_consistency.md
[ConsistencyStrategy]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[ReplSize]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md