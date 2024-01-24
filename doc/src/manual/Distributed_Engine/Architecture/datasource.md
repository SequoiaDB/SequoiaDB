SequoiaDB 巨杉数据库实现了数据的分布式管理，应用程序可以通过丰富的接口对存储在同一集群中的数据进行灵活访问。但在一些大型的系统中，用户会使用多个 SequoiaDB 集群进行数据存储，同时需要访问多个集群中的数据，因此 SequoiaDB 提供了多数据源的功能。

##逻辑架构##

数据源支持通过同一入口，实现跨集群的数据访问，其架构图如下：

![数据源架构图][image]

用户在使用数据源功能前，需要先在当前使用的集群中创建数据源，并指定数据源的相关属性。然后在创建集合或集合空间时指定该数据源，使其映射至数据源中对应的集合或集合空间。成功建立映射关系后，用户可以在当前集合中对源数据集合进行数据访问。

> **Note:**
>
> 数据源的相关属性可参考 [SYSDATASOURCES 集合][datasource]。

##注意事项##

当前，数据源支持对集合的基本操作包括 CRUD、 truncate、count、explain、Lob 等，不支持 DDL 操作。用户可通过参数控制报错级别，忽略执行不支持的数据操作时产生的错误信息。

具体操作可参考[搭建数据源][build]。



[^_^]:
    本文使用的所有引用和链接
[image]:images/Distributed_Engine/Architecture/datasource.png
[build]:manual/Distributed_Engine/Operation/build_datasource.md
[datasource]:manual/Manual/Catalog_Table/SYSDATASOURCES.md
