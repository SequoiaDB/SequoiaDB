[^_^]:
    Sharding首页
    作者：林友滨
    时间：20190222
    评审意见
    林友滨：初稿完成；时间：20190222
    市场部：时间：20190604


在大数据的场景下，用户的业务数据往往有几千万甚至上亿条记录。在这种场景下，一次简单的查询请求往往需要读取大量的磁盘数据，造成 I/O 的高负载和高时延问题。而在 SequoiaDB 巨杉数据库中，通过使用数据分区可以大大减少读取的数据量，提高数据查询的并发度，从而解决了 I/O 的高负载和高时延问题。

通过本章文档，用户可以了解分区的基本概念和原理，熟悉数据分区的基本操作。使用数据分区，是提高系统性能的重要手段。本章主要内容如下：

+ [分区原理][sharding_architecture]
+ [分区索引][sharding_index]
+ [分区配置][sharding_config]
+ [分区键选择][sharding_keys]
+ [多维分区][sharding_multi_dimension]
+ [数据分区操作实例][example]

[^_^]:
    本文使用到的所有链接及引用。
[sharding_architecture]:manual/Distributed_Engine/Architecture/Sharding/architecture.md
[sharding_index]:manual/Distributed_Engine/Architecture/Sharding/sharding_index.md
[sharding_config]:manual/Distributed_Engine/Architecture/Sharding/config.md
[sharding_keys]:manual/Distributed_Engine/Architecture/Sharding/sharding_keys.md
[sharding_multi_dimension]:manual/Distributed_Engine/Architecture/Sharding/multi_dimension.md
[example]:manual/Distributed_Engine/Architecture/Sharding/example.md