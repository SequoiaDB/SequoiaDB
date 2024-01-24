  [^_^]:
      JavaScript 驱动

JavaScript 驱动提供了数据库操作和集群操作的接口。主要包括数据库、集合空间、集合、游标、复制组、节点、域和大对象级别的操作。

##操作##

SequoiaDB 巨杉数据库通过 SDB Shell 使用户能够以命令行方式使用 JavaScript 语法与 SequoiaDB 的分布式引擎进行交互。SDB Shell 入门教程可参考[快速部署][quick_deployment]章节，深入了解 Shell 模式的内置方法可参考 [SequoiaDB Shell 方法][SequoiaDB_Shell]章节。

##数据类型##

JSON 实例支持丰富的数据类型，包括：

* String
* Int
* Double
* Decimal
* Date
* Timestamp
* Binary
* MaxKey
* MinKey
* BSONObject
* Array
* Boolean
* ObjectId

用户可以通过 [SequoiaDB 支持数据类型][data_type]了解更多详细信息。



[^_^]:
     本文使用的所有引用和链接
[data_type]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md
[SequoiaDB_Shell]:manual/Manual/Sequoiadb_Command/Readme.md
[quick_deployment]:manual/Quick_Start/quick_deployment.md#基本操作