[^_^]:
    概述

在 SequoiaDB 巨杉数据库中，涉及数据查询的操作都会用到访问计划，包括数据的查询、更新和删除操作。在集群部署模式下，请求主要由协调节点和数据节点协作完成处理。

协调节点的主要任务包括：

+ 根据查询条件提取集合分区键，结合集合的编目信息确定需要下发请求的复制组
+ 对各数据节点返回的结果集进行聚合（如排序等），并返回给客户端

数据节点的主要任务包括：

+ 对请求进行解析，结合查询条件和排序字段，通过基于代价的估算选取合适的访问计划（全表扫描，或使用集合上的某个索引进行索引扫描）
+ 调用存储引擎进行数据读取，并按要求进行数据处理（如排序）
+ 将结果返回给协调节点
+ 生成的访问计划可以被缓存，以供新的查询重用，提高查询性能

SequoiaDB 实现了 CBO，并且提供了灵活的访问计划缓存配置能力，用户可根据实际业务场景进行最优配置。

本章主要讲述访问计划相关的基本概念和原理，内容包含：

+ [统计信息][statistics]
+ [基于代价的访问计划评估][cost_estimation]
+ [访问计划缓存][explain]
+ [查看计划缓存][cache]



[^_^]:
    本文使用到的所有内部链接及引用。
[statistics]:manual/Distributed_Engine/Maintainance/Access_Plan/statistics.md
[cost_estimation]:manual/Distributed_Engine/Maintainance/Access_Plan/cost_estimation.md
[explain]:manual/Distributed_Engine/Maintainance/Access_Plan/explain.md
[cache]:manual/Distributed_Engine/Maintainance/Access_Plan/cache.md