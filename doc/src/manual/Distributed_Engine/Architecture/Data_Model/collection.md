[^_^]:
    集合
    作者：谭钊波
    时间：20190701
    评审意见
    王涛：
    许建辉：
    市场部：20190822


概念
----
集合（Collection）又称为表（Table），是数据库中存放记录的逻辑对象。任何一条记录属于且仅属于一个集合。集合由“<集合空间名>.<集合名>”作为唯一标示，其中集合名最大长度为 127 字节，且需为 UTF-8 编码。一个集合可以包含零条或者多条记录（上限为集合空间大小）。

属性
----
在集群环境下，每个集合拥有除名称外的以下属性：

| 属性名                   | 描述 |
| ------------------------ | ---- |
| 分区键（ShardingKey）    | 指定集合的分区键，集合中所有的记录将分区键中指定的字段作为分区信息，分别存放在所对应的分区中|
| 分区类型（ShardingType） | 指定集合的分区类型：范围分区（range）或散列分区（hash）|
| 分区数（Partition） | 仅当选择散列分区时填写，代表了散列分区的个数 |
| 写副本数（ReplSize）     | 指定该集合默认的写副本数 |
| 同步一致性策略（ConsistencyStrategy）| 指定该集合的[同步一致性][ConsistencyStrategy]策略 |
| 数据压缩（Compressed）   | 指定新集合是否开启数据压缩功能 |
| 压缩算法（CompressionType）| 压缩算法类型 |
| 主子表（IsMainCL）| 指定新集合是否为主分区集合 |
| 自动切分（AutoSplit）| 指定新集合是否开启自动切分功能 |
| 集合属组（Group）| 指定新集合将被创建到哪个复制组 |
| $id 索引（AutoIndexId）| 指定新集合是否自动使用 _id 字段创建名字为"$id"的唯一索引 |
| $shard 索引（EnsureShardingIndex）| 指定集合是否自动使用 ShardingKey 包含的字段创建名字为"$shard"的索引 |
| 严格数据模式（StrictDataMode）| 指定对该集合的操作是否开启严格数据类型模式 |
| 自增字段（AutoIncrement）| 自增字段 |

> **Note:**
>
> 关于集合属性及属性的取值可参考 [SdbCS.createCL()][data_mode_createCL]。

其他相关
----
在分布式集群中，集合一般需要结合数据分区使用，详情可参考[数据分区][data_mode_split]章节。


[^_^]:
    本文使用到的所有链接及引用
[data_mode_createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[data_mode_split]:manual/Distributed_Engine/Architecture/Sharding/Readme.md
[ConsistencyStrategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md
