编目节点是一种逻辑节点，用于保存数据库的元数据信息，而不保存用户数据。

编目节点属于编目复制组（可参考[复制组][Replication]）。

编目节点中包含以下集合空间：

- **SYSCAT：** 系统编目集合空间，包含以下系统集合：

    | 集合名            |             描述                         |
    |-------------------|------------------------------------------|
    | [SYSCOLLECTIONS][syscollctions] | 保存了该集群中所有的用户集合信息 |
    | [SYSCOLLECTIONSPACES][syscollectionspaces] | 保存了该集群中所有的用户集合空间信息 |
    | [SYSDOMAINS][sysdomains] | 保存了该集群中所有用户域的信息 |
    | [SYSNODES][sysnodes] | 保存了该集群中所有的逻辑节点与复制组信息 |
    | [SYSTASKS][systasks] | 保存了该集群中所有正在运行的后台任务信息 |
    | [SYSDATASOURCES][sysdatasource] | 保存了该集群中所有数据源的元数据信息 |

- **SYSAUTH：** 系统认证集合空间，包含一个用户集合，保存当前系统中所有的用户信息

    |集合名          |    描述                      |
    |----------------|------------------------------|
    | [SYSUSRS][sysusrs] | 保存了该集群中所有的用户信息 |

- **SYSPROCEDURES：** 系统存储过程集合空间，包含一个集合，用于存储所有的存储过程函数信息

    |    集合名      |   描述                   |
    |----------------|--------------------------|
    | [STOREPROCEDURES][storeprocedures] | 保存所有存储过程函数信息 |

- **SYSGTS：** 系统自增字段集合空间，包含一个集合，用于存储所有的自增字段信息

    |    集合名      |   描述                   |
    |----------------|--------------------------|
    | [SEQUENCES][sequences] | 保存所有自增字段信息 |

除编目节点外，集群中所有其他的节点不在磁盘中保存任何全局元数据信息。当需要访问其他节点上的数据时，除编目节点外的其他节点需要从本地缓存中寻找集合信息，如果不存在则需要从编目节点获取。

编目节点与其它节点之间主要使用编目服务端口（catalogname 参数）进行通讯。

## 管理编目节点


在新建编目复制组和新增编目节点时，如果涉及到新增主机，则需要先按照[集群中新增主机][expand]一节完成主机的主机名和参数配置。

### 新建编目复制组

一个数据库集群必须有且仅有一个编目复制组。

操作方法：

```lang-javascript
> db.createCataRG( <host>, <service>, <dbpath>, [config] )
```

 [Sdb.createCataRG()][createCataRG] 用于创建编目复制组，同时创建并启动一个编目节点，其中：

  - **host** ：指定编目节点的主机名；

  - **service** ：指定编目节点的服务端口，需要确保该端口号及往后延续的五个端口号未被占用；如端口号设置为 11800，应确保 11800/11801/11802/11803/11804/11805 端口都未被占用；

  - **dbpath** ：指定编目节点数据文件的存放路径，必须输入绝对路径，且需要确保数据管理员（安装时创建，默认为sdbadmin）用户在该路径下有写权限；

  - **config** ：该参数为可选参数，用于指定节点的配置信息。如配置日志大小、是否打开事务等，配置格式应为 json 格式，具体配置可参考[参数说明][cluster_config]。

### 编目复制组中新增节点

随着整个集群中的物理设备的扩展，用户可以通过增加编目节点来提高编目服务的可靠性。

操作方法：

1. 获取编目复制组

    ```lang-javascript
    > var cataRG = db.getCatalogRG()
    ```
    
    >   **Note:**
    >
    >   在 SDB Shell 中用户可以使用 [Sdb.getCataRG()][getCataRG] 获取编目复制组。

2. 创建一个新的编目节点

    ```lang-javascript
    > var node1 = cataRG.createNode( <host>, <service>, <dbpath>, [config] )
    ```
    
    >   **Note:**
    >
    >   **host**、 **service**、 **dbpath** 及 **config** 的设置可参考[新建编目复制组][create_group]。

3. 启动新增的编目节点

    ```lang-javascript
    > node1.start()
    ```

### 查看编目节点

在 SDB Shell 中查看编目复制组的详细信息

```lang-javascript
> db.getCataRG().getDetail()
```

## 故障恢复

编目节点故障恢复策略与[数据节点][data_node]相同。


[^_^]:
     本文使用的所有引用和链接
[Replication]:manual/Distributed_Engine/Architecture/Replication/architecture.md
[syscollctions]:manual/Manual/Catalog_Table/SYSCOLLECTIONS.md
[syscollectionspaces]:manual/Manual/Catalog_Table/SYSCOLLECTIONSPACES.md
[sysdomains]:manual/Manual/Catalog_Table/SYSDOMAINS.md
[sysnodes]:manual/Manual/Catalog_Table/SYSNODES.md
[systasks]:manual/Manual/Catalog_Table/SYSTASKS.md
[sysusrs]:manual/Manual/Catalog_Table/SYSUSRS.md
[sysdatasource]:manual/Manual/Catalog_Table/SYSDATASOURCES.md
[storeprocedures]:manual/Manual/Catalog_Table/STOREPROCEDURES.md
[sequences]:manual/Manual/Catalog_Table/SEQUENCES.md
[expand]:manual/Distributed_Engine/Maintainance/Expand/expand.md
[cluster_config]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[createCataRG]:manual/Manual/Sequoiadb_Command/Sdb/createCataRG.md
[getCataRG]:manual/Manual/Sequoiadb_Command/Sdb/getCataRG.md
[create_group]:manual/Distributed_Engine/Architecture/Node/catalog_node.md#新建编目复制组
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md