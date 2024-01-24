编目节点中包含以下集合空间：

- SYSCAT：系统编目集合空间，包含以下系统集合：

    | 集合名            |             描述                         |
    |-------------------|------------------------------------------|
    | [SYSCOLLECTIONS][syscollctions] | 保存了该集群中所有的用户集合信息 |
    | [SYSCOLLECTIONSPACES][syscollectionspaces] | 保存了该集群中所有的用户集合空间信息 |
    | [SYSDOMAINS][sysdomains] | 保存了该集群中所有用户域的信息 |
    | [SYSNODES][sysnodes] | 保存了该集群中所有的逻辑节点与复制组信息 |
    | [SYSTASKS][systasks] | 保存了该集群中所有正在运行的后台任务信息 |
    | [SYSDATASOURCES][sysdatasource] | 保存了该集群中所有数据源的元数据信息 |
    | [SYSGROUPMODES][sysgroupmodes] | 保存了该集群中所有复制组的运行模式 |

- SYSAUTH：系统认证集合空间，包含一个用户集合，保存当前系统中所有的用户信息

    |集合名          |    描述                      |
    |----------------|------------------------------|
    | [SYSUSRS][sysusrs] | 保存了该集群中所有的用户信息 |

- SYSPROCEDURES：系统存储过程集合空间，包含一个集合，用于存储所有的存储过程函数信息

    |    集合名      |   描述                   |
    |----------------|--------------------------|
    | [STOREPROCEDURES][storeprocedures] | 保存所有存储过程函数信息 |

- SYSGTS：系统自增字段集合空间，包含一个集合，用于存储所有的自增字段信息

    |    集合名      |   描述                   |
    |----------------|--------------------------|
    | [SEQUENCES][sequences] | 保存所有自增字段信息 |

用户可通过如下步骤查看编目表中的信息：

1. 连接编目节点

    ```lang-javascript
    > var cata = new Sdb("localhost", 11800)
    ```

2. 查看该集群中所有的用户集合信息

    ```lang-javascript
    > cata.SYSCAT.SYSCOLLECTIONS.find()
    ```


[^_^]:
     本文使用的所有引用和链接
[syscollctions]:manual/Manual/Catalog_Table/SYSCOLLECTIONS.md
[syscollectionspaces]:manual/Manual/Catalog_Table/SYSCOLLECTIONSPACES.md
[sysdomains]:manual/Manual/Catalog_Table/SYSDOMAINS.md
[sysnodes]:manual/Manual/Catalog_Table/SYSNODES.md
[systasks]:manual/Manual/Catalog_Table/SYSTASKS.md
[sysusrs]:manual/Manual/Catalog_Table/SYSUSRS.md
[sysdatasource]:manual/Manual/Catalog_Table/SYSDATASOURCES.md
[sysgroupmodes]:manual/Manual/Catalog_Table/SYSGROUPMODES.md
[storeprocedures]:manual/Manual/Catalog_Table/STOREPROCEDURES.md
[sequences]:manual/Manual/Catalog_Table/SEQUENCES.md