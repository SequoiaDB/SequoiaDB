随着用户、数据量和交易量的大幅度提升，导致交易高峰压力及交易复杂度急剧增加，传统的集中式关系数据库在可扩展性上的弊端愈发显著。

SequoiaDB 巨杉数据库通过独创的引擎级多模架构，实现跨引擎 ACID 事务一致性。与众多云原生数据库一样，SequoiaDB 提供存储计算分离的云服务架构，并支持在此架构上构建包括：MySQL、PostgreSQL、Spark 等多种数据库引擎。客户可以灵活选择需要的数据库引擎，实现无须代码修改的平滑迁移。不同的是，SequoiaDB 底层并非构建于一个简单的分布式存储或计算资源平台，其分布式引擎层是一个完整的数据库，具备事务一致性、排序过滤、下推计算等能力。

同时，SequoiaDB 的分布式引擎层提供了所有数据的多副本管理，为上层计算实例层构建逻辑“数据域”，每个数据区可以实现租户隔离。

本章主要介绍 SequoiaDB 分布式引擎层的系统架构、内核原理和运维管理等相关内容：

- [系统架构][Architecture]
- [操作指南][Operation]
- [运维管理][Maintainance]



[^_^]:
   本文使用的所有引用及链接
[Architecture]:manual/Distributed_Engine/Architecture/Readme.md
[Operation]:manual/Distributed_Engine/Operation/collectionspace_operation.md
[Maintainance]:manual/Distributed_Engine/Maintainance/Readme.md
