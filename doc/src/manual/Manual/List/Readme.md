
在 SequoiaDB 巨杉数据库中，列表是一种轻量级的得到系统当前状态的命令，主要分为以下类型：

| 列表标识 | 列表类型 | 描述 |
| -------- | -------- | ---- |
| [SDB_LIST_CONTEXTS][SDB_LIST_CONTEXTS] | 上下文列表 | 列出当前数据库节点中所有的会话所对应的上下文 |
| [SDB_LIST_CONTEXTS_CURRENT][SDB_LIST_CONTEXTS_CURRENT] | 当前会话上下文列表 | 列出当前数据库节点中当前会话所对应的上下文 |
| [SDB_LIST_SESSIONS][SDB_LIST_SESSIONS] | 会话列表 | 列出当前数据库节点中所有的会话 |
| [SDB_LIST_SESSIONS_CURRENT][SDB_LIST_SESSIONS_CURRENT] | 当前会话列表 | 列出当前数据库节点中当前的会话 |
| [SDB_LIST_COLLECTIONS][SDB_LIST_COLLECTIONS] | 集合列表 | 列出当前数据库节点或集群中所有非临时集合 |
| [SDB_LIST_COLLECTIONSPACES][SDB_LIST_COLLECTIONSPACES] | 集合空间列表 | 列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [SDB_LIST_STORAGEUNITS][SDB_LIST_STORAGEUNITS] | 存储单元列表 | 列出当前数据库节点的全部存储单元信息 |
| [SDB_LIST_GROUPS][SDB_LIST_GROUPS] | 分区组列表 | 列出当前集群中的所有分区信息 |
| [SDB_LIST_TASKS][SDB_LIST_TASKS] | 后台任务列表 | 列出正在运行的后台任务信息 |
| [SDB_LIST_TRANSACTIONS][SDB_LIST_TRANSACTIONS] | 事务列表 | 列出数据库中正在进行的事务信息 |
| [SDB_LIST_TRANSACTIONS_CURRENT][SDB_LIST_TRANSACTIONS_CURRENT]| 当前事务列表 | 列出当前会话正在进行的事务信息 |
| [SDB_LIST_SVCTASKS][SDB_LIST_SVCTASKS] | 服务任务列表 | 列出当前数据库节点中所有的服务任务 |
| [SDB_LIST_SEQUENCES][SDB_LIST_SEQUENCES] | 序列列表 | 列出当前数据库中所有的序列信息 |
| [SDB_LIST_USERS][SDB_LIST_USERS]| 用户列表 | 列出当前集群中的所有用户信息 |
| [SDB_LIST_BACKUPS][SDB_LIST_BACKUPS] | 备份列表 | 列出当前数据库的备份信息 |
| [SDB_LIST_DATASOURCES][SDB_LIST_DATASOURCES] | 数据源列表 | 列出当前数据库中所有数据源的元数据信息 |
| [SDB_LIST_RECYCLEBIN][SDB_LIST_RECYCLEBIN] | 回收站项目列表 | 列出当前回收站中已回收项目的元数据信息 |
| [SDB_LIST_GROUPMODES][SDB_LIST_GROUPMODES] | 复制组运行模式列表 | 列出当前集群中所有复制组的运行模式 |

>   **Note:**
>
>   用户可以通过调用 [list()][list] 获取列表。



[^_^]:
     本文使用的所有引用及链接
[SDB_LIST_CONTEXTS]:manual/Manual/List/SDB_LIST_CONTEXTS.md
[SDB_LIST_CONTEXTS_CURRENT]:manual/Manual/List/SDB_LIST_CONTEXTS_CURRENT.md
[SDB_LIST_SESSIONS]:manual/Manual/List/SDB_LIST_SESSIONS.md
[SDB_LIST_SESSIONS_CURRENT]:manual/Manual/List/SDB_LIST_SESSIONS_CURRENT.md
[SDB_LIST_COLLECTIONS]:manual/Manual/List/SDB_LIST_COLLECTIONS.md
[SDB_LIST_COLLECTIONSPACES]:manual/Manual/List/SDB_LIST_COLLECTIONSPACES.md
[SDB_LIST_STORAGEUNITS]:manual/Manual/List/SDB_LIST_STORAGEUNITS.md
[SDB_LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
[SDB_LIST_TRANSACTIONS]:manual/Manual/List/SDB_LIST_TRANSACTIONS.md
[SDB_LIST_TRANSACTIONS_CURRENT]:manual/Manual/List/SDB_LIST_TRANSACTIONS_CURRENT.md
[SDB_LIST_SVCTASKS]:manual/Manual/List/SDB_LIST_SVCTASKS.md
[SDB_LIST_SEQUENCES]:manual/Manual/List/SDB_LIST_SEQUENCES.md
[SDB_LIST_USERS]:manual/Manual/List/SDB_LIST_USERS.md
[SDB_LIST_BACKUPS]:manual/Manual/List/SDB_LIST_BACKUPS.md
[SDB_LIST_DATASOURCES]:manual/Manual/List/SDB_LIST_DATASOURCES.md
[SDB_LIST_TASKS]:manual/Manual/List/SDB_LIST_TASKS.md
[SDB_LIST_RECYCLEBIN]:manual/Manual/List/SDB_LIST_RECYCLEBIN.md
[SDB_LIST_GROUPMODES]:manual/Manual/List/SDB_LIST_GROUPMODES.md
[list]:manual/Manual/Sequoiadb_Command/Sdb/list.md

