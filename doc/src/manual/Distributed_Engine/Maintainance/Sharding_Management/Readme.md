[^_^]:
    分区管理
    作者：魏彰凯
    时间：20190516
    评审意见
    王涛：
    许建辉：
    市场部：20190604
       

在 SequoiaDB 巨杉数据库中，通过使用[数据分区][sharding_architecture]，可以大大减少单次查询读取的数据量，提高数据查询效率。合理的数据分区是优化查询效率的绝佳途径，而不规范地使用数据分区，反而会降低查询效率。本文档将介绍分区管理的策略，帮助用户合理管理分区。

数据分区管理，是对数据集群建设的规划和现有集群的维护，用户将从以下几个方面了解数据分区管理策略：

+ [分区数据均衡][sharding_balance]：描述分区数据均衡性的监控及调优
+ [避免热点数据][sharding_avoid_hot]：描述热点数据的危害及避免实践
+ [分区可靠性管理][sharding_reliability]：描述数据可靠性的选择及实践
+ [分区性能监控][sharding_monitoring]：描述数据分区的性能及健康监测

[^_^]:
    本文使用到的所有链接
    
[sharding_architecture]:manual/Distributed_Engine/Architecture/Sharding/architecture.md
[sharding_balance]:manual/Distributed_Engine/Maintainance/Sharding_Management/balance.md
[sharding_avoid_hot]:manual/Distributed_Engine/Maintainance/Sharding_Management/avoid_hot.md
[sharding_reliability]:manual/Distributed_Engine/Maintainance/Sharding_Management/reliability.md
[sharding_monitoring]:manual/Distributed_Engine/Maintainance/Sharding_Management/monitoring.md