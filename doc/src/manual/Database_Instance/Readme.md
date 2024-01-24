[^_^]:
    数据库实例概述


SequoiaDB 巨杉数据库采用计算存储分离架构。数据库底层以支持分布式事务能力的存储节点构建可横向扩展的存储集群，上层通过创建多实例的方式提供 MySQL、MariaDB、PostgreSQL、以及 SparkSQL 的支持。除了支持结构化 SQL 实例以外，SequoiaDB 巨杉数据库还支持创建 JSON 和 S3对象存储实例。

![structure][structure]

SequoiaDB 巨杉数据库的分布式架构一方面可以提供针对数据表的无限横向水平扩张，另一方面在计算层通过提供不同类型数据库实例的方式，100% 兼容 MySQL、MariaDB、PostgreSQL 与 SparkSQL，原生支持跨表跨节点分布式事务能力，应用程序基本可以在零改动的基础上进行数据库迁移。

除了结构化数据外，SequoiaDB 巨杉数据库可以在同一集群支持包括 JSON 和 S3 对象存储在内的非结构化数据，使整个数据库面向上层的微服务架构应用提供了完整的数据服务资源池。

本文档将主要介绍 SequoiaDB 所支持的三类数据库实例的操作和开发：

- 关系型数据库实例
  - [MySQL 实例][mysql]
  - [MariaDB 实例][mariadb]
  - [PostgreSQL 实例][pgsql]
  - [SparkSQL 实例][sparksql]
  - [FlinkSQL 连接器][flinksql]
- [JSON 实例][json]
- 对象存储实例 
  - [S3 对象存储实例][s3]

[^_^]:
    本文使用的所有引用及链接
[structure]:images/Database_Instance/structure.png
[mysql]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Readme.md
[pgsql]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Readme.md
[sparksql]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Readme.md
[json]:manual/Database_Instance/Json_Instance/Readme.md
[s3]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/Readme.md
[mariadb]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Readme.md
[flinksql]:manual/Database_Instance/Relational_Instance/FlinkSQL_Connector/Readme.md