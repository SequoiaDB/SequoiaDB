
监控是一种监视当前系统状态的方式。在 SequoiaDB 巨杉数据库中，用户可以使用快照（SNAPSHOT）与列表（LIST）命令进行系统监控。

##快照视图##

快照是一种得到系统当前状态的命令，主要分为以下类型：

| 快照标识 | 对应 sdbshell 接口标识 | 快照类型 | 描述 |
| -------- | -------- | -------- | ---- |
| [$SNAPSHOT_CONTEXT][SNAPSHOT_CONTEXT] | [SDB_SNAP_CONTEXTS][SDB_SNAP_CONTEXTS] | 上下文快照 | 上下文快照列出当前数据库节点中所有的会话所对应的上下文 |
| [$SNAPSHOT_CONTEXT_CUR][SNAPSHOT_CONTEXT_CUR] | [SDB_SNAP_CONTEXTS_CURRENT][SDB_SNAP_CONTEXTS_CURRENT] | 当前会话上下文快照 | 当前上下文快照列出当前数据库节点中当前会话所对应的上下文 |
| [$SNAPSHOT_SESSION][SNAPSHOT_SESSION] | [SDB_SNAP_SESSIONS][SDB_SNAP_SESSIONS] | 会话快照 | 会话快照列出当前数据库节点中所有的会话 |
| [$SNAPSHOT_SESSION_CUR][SNAPSHOT_SESSION_CUR] | [SDB_SNAP_SESSIONS_CURRENT][SDB_SNAP_SESSIONS_CURRENT] | 当前会话快照 | 当前会话快照列出当前数据库节点中当前的会话 |
| [$SNAPSHOT_CL][SNAPSHOT_CL] | [SDB_SNAP_COLLECTIONS][SDB_SNAP_COLLECTIONS] | 集合快照 | 集合快照列出当前数据库节点或集群中所有非临时集合 |
| [$SNAPSHOT_CS][SNAPSHOT_CS] | [SDB_SNAP_COLLECTIONSPACES][SDB_SNAP_COLLECTIONSPACES] | 集合空间快照 | 集合空间快照列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [$SNAPSHOT_DB][SNAPSHOT_DB] | [SDB_SNAP_DATABASE][SDB_SNAP_DATABASE] | 数据库快照 | 数据库快照列出当前数据库节点的数据库监视信息 |
| [$SNAPSHOT_SYSTEM][SNAPSHOT_SYSTEM] | [SDB_SNAP_SYSTEM][DB_SNAP_SYSTEM] | 系统快照 | 系统快照列出当前数据库节点的系统监视信息 |
| [$SNAPSHOT_CATA][SNAPSHOT_CATA] | [SDB_SNAP_CATALOG][SDB_SNAP_CATALOG] | 编目信息快照 | 用于查看编目信息 |
| [$SNAPSHOT_TRANS][SNAPSHOT_TRANS] | [SDB_SNAP_TRANSACTIONS][SDB_SNAP_TRANSACTIONS] | 事务快照 | 事务快照列出数据库中正在进行的事务信息 |
| [$SNAPSHOT_TRANS_CUR][SNAPSHOT_TRANS_CUR] | [SDB_SNAP_TRANSACTIONS_CURRENT][SDB_SNAP_TRANSACTIONS_CURRENT] | 当前事务快照 | 当前事务快照列出当前会话正在进行的事务信息 |
| [$SNAPSHOT_ACCESSPLANS][SNAPSHOT_ACCESSPLANS] | [SDB_SNAP_ACCESSPLANS][SDB_SNAP_ACCESSPLANS] | 访问计划缓存快照 | 访问计划缓存快照列出数据库中缓存的访问计划的信息 |
| [$SNAPSHOT_HEALTH][SNAPSHOT_HEALTH] | [SDB_SNAP_HEALTH][SDB_SNAP_HEALTH] | 节点健康检测快照 | 节点健康检测快照列出数据库中所有节点的健康信息 |
| [$SNAPSHOT_CONFIGS][SNAPSHOT_CONFIGS] | [SDB_SNAP_CONFIGS][SDB_SNAP_CONFIGS] | 配置快照 | 配置快照列出数据库中指定节点的配置信息 |
| [$SNAPSHOT_SEQUENCES][SNAPSHOT_SEQUENCES] | [SDB_SNAP_SEQUENCES][SDB_SNAP_SEQUENCES] | 序列快照 | 序列快照列出当前数据库的全部序列信息 |
| [$SNAPSHOT_SVCTASKS][SNAPSHOT_SVCTASKS] | [SDB_SNAP_SVCTASKS][SDB_SNAP_SVCTASKS] | 服务任务快照 | 服务任务快照列出当前数据库节点中服务任务的统计信息 |
| [$SNAPSHOT_QUERIES][SNAPSHOT_QUERIES] | [SDB_SNAP_QUERIES][SDB_SNAP_QUERIES] | 查询快照 | 查询快照列出当前数据库节点中查询信息 |
| [$SNAPSHOT_LOCKWAITS][SNAPSHOT_LOCKWAITS] | [SDB_SNAP_LOCKWAITS][SDB_SNAP_LOCKWAITS] | 锁等待快照 | 等待锁快照列出当前数据库节点中锁等待信息 |
| [$SNAPSHOT_LATCHWAITS][SNAPSHOT_LATCHWAITS] | [SDB_SNAP_LATCHWAITS][SDB_SNAP_LATCHWAITS] | 闩锁等待快照 | 闩锁等待快照列出当前数据库节点中闩锁等待信息 |
| [$SNAPSHOT_INDEXSTATS][SNAPSHOT_INDEXSTATS] | [SDB_SNAP_INDEXSTATS][SDB_SNAP_INDEXSTATS] | 索引统计信息快照 | 索引统计信息快照列出当前数据库中所有索引统计信息 |
| [$SNAPSHOT_TRANSWAIT][SNAPSHOT_TRANSWAIT] | [SDB_SNAP_TRANSWAITS][SDB_SNAP_TRANSWAITS] | 事务等待快照 | 事务等待快照可以列出数据库中因锁等待而产生的事务等待信息 |
| [$SNAPSHOT_TRANSDEADLOCK][SNAPSHOT_TRANSDEADLOCK] | [SDB_SNAP_TRANSDEADLOCK][SDB_SNAP_TRANSDEADLOCK] | 事务死锁检测快照 | 事务死锁检测快照可以列出数据库中处于死锁状态的事务信息 |
| [$SNAPSHOT_RECYCLEBIN][SNAPSHOT_RECYCLEBIN] | [SDB_SNAP_RECYCLEBIN][SDB_SNAP_RECYCLEBIN] | 回收站项目快照 | 回收站项目快照可以列出当前回收站中已回收项目的详细信息 |

##列表视图##

列表是一种轻量级的得到系统当前状态的命令，主要分为以下类型：

| 列表标示 | 对应 sdbshell 接口标示 | 列表类型 | 描述 |
| -------- | -------- | -------- | ---- |
| [$LIST_CONTEXT][LIST_CONTEXT] | [SDB_LIST_CONTEXTS][SDB_LIST_CONTEXTS] | 上下文列表 | 上下文列表列出当前数据库节点中所有的会话所对应的上下文 |
| [$LIST_CONTEXT_CUR][LIST_CONTEXT_CUR] | [SDB_LIST_CONTEXTS_CURRENT][SDB_LIST_CONTEXTS_CURRENT] | 当前会话上下文列表 | 当前上下文列表列出当前数据库节点中当前会话所对应的上下文 |
| [$LIST_SESSION][LIST_SESSION] | [SDB_LIST_SESSIONS][SDB_LIST_SESSIONS] | 会话列表 | 会话列表列出当前数据库节点中所有的会话 |
| [$LIST_SESSION_CUR][LIST_SESSION_CUR] | [SDB_LIST_SESSIONS_CURRENT][SDB_LIST_SESSIONS_CURRENT] | 当前会话列表 | 当前会话列表列出当前数据库节点中当前的会话 |
| [$LIST_CL][LIST_CL] | [SDB_LIST_COLLECTIONS][SDB_LIST_COLLECTIONS] | 集合列表 | 集合列表列出当前数据库节点或集群中所有非临时集合 |
| [$LIST_CS][LIST_CS] | [SDB_LIST_COLLECTIONSPACES][SDB_LIST_COLLECTIONSPACES] | 集合空间列表 | 集合空间列表列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [$LIST_SU][LIST_SU] | [SDB_LIST_STORAGEUNITS][SDB_LIST_STORAGEUNITS]  | 存储单元列表 | 存储单元列表列出当前数据库节点的全部存储单元信息 |
| [$LIST_GROUP][LIST_GROUP]  | [SDB_LIST_GROUPS][SDB_LIST_GROUPS] | 复制组列表 | 复制组列表列出当前集群中的所有数据分区信息 |
| [$LIST_GROUPMODES][LIST_GROUPMODES]  | [SDB_LIST_GROUPMODES][SDB_LIST_GROUPMODES] | 复制组运行模式列表 | 复制组运行模式列表列出当前集群中的所有复制组的运行模式 |
| [$LIST_TRANS][LIST_TRANS] | [SDB_LIST_TRANSACTIONS][SDB_LIST_TRANSACTIONS] | 事务列表 | 事务列表列出数据库中正在进行的事务信息 |
| [$LIST_TRANS_CUR][LIST_TRANS_CUR] | [SDB_LIST_TRANSACTIONS_CURRENT][SDB_LIST_TRANSACTIONS_CURRENT] | 当前事务列表 | 当前事务列表列出当前会话正在进行的事务信息 |
| [$LIST_SEQUENCES][LIST_SEQUENCES]  | [SDB_LIST_SEQUENCES][SDB_LIST_SEQUENCES] | 序列列表 | 序列列表列出当前数据库中所有的序列信息 |
| [$LIST_BACKUP][LIST_BACKUP] | [SDB_LIST_BACKUPS][SDB_LIST_BACKUPS] | 备份列表 | 备份列表列出当前数据库的备份信息 |
| [$LIST_SVCTASKS][LIST_SVCTASKS]  | [SDB_LIST_SVCTASKS][SDB_LIST_SVCTASKS] | 服务任务列表 | 服务任务列表列出当前数据库节点中所有的服务任务 |
| [$LIST_USER][LIST_USER] | [SDB_LIST_USERS][SDB_LIST_USERS] | 用户列表 | 用户列表列出当前集群中的所有用户信息 |
| [$LIST_DATASOURCE][LIST_DATASOURCE] | [SDB_LIST_DATASOURCES][SDB_LIST_DATASOURCES] | 数据源列表 | 数据源列表列出当前数据库中所有数据源的元数据信息 |
| [$LIST_RECYCLEBIN][LIST_RECYCLEBIN] | [SDB_LIST_RECYCLEBIN][SDB_LIST_RECYCLEBIN] | 回收站项目列表 | 回收站项目列表可以列出当前回收站中已回收项目的元数据信息 |

##SQL到SequoiaDB映射表##

下表列出了 SQL 快照查询语句的操作在 API 中对应的[快照操作][snapshot]：

| SQL 语句 | API 语句        |
| -------- | -------------- |
|  select \<sel\> from $\<snapshot\> where \<cond\> order by \<sort\>  |   db.snapshot( <snapType>, [cond], [sel], [sort] ) |
| db.exec( "select * from $SNAPSHOT_CONTEXT where SessionID = 20" ) | 过滤指定条件的记录。db.snapshot(SDB_SNAP_CONTEXTS, { SessionID: 20 } ) |
| db.exec( " select NodeName from $SNAPSHOT_CONTEXT " ) | 只显示记录的指定字段。db.snapshot(SDB_SNAP_CONTEXTS, {}, { NodeName:""} ) |
| db.exec( " select * from $SNAPSHOT_CONTEXT order by SessionID" ) | 根据指定字段进行排序。db.snapshot(SDB_SNAP_CONTEXTS, {}, {}, { "SessionID": 1 } ) |

下面列出了 SQL 快照查询语句的操作在 API 中使用指定[快照查询参数][SdbSnapshotOption]的对应快照操作：

```
select <sel> from $<snapshot>
               where <cond>
               order by <sort>
               limit <limit>
               offset <skip> /*+use_option(<options>)*/
```

对应

```
SdbSnapshotOption[.cond(<cond>)]
                 [.sel(<sel>)]
                 [.sort(<sort>)]
                 [.options(<options>)]
                 [.skip(<skip>)]
                 [.limit(<limit>)]
```

###cond(\<cond\>)###

对应 SQL 语法 [where 子句][where]

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( "select * from $SNAPSHOT_CONTEXT where SessionID = 22" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( { SessionID: 22 } ) ) |

###sel(\<sel\>)###

对应 SQL 语法的字段名

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( "select SessionID from $SNAPSHOT_CONTEXT" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).sel( { SessionID: "" } ) ) |

###sort(\<sort\>)###

对应 SQL 语法 [order by 子句][order_by]

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( " select * from $SNAPSHOT_CONTEXT order by SessionID" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).sort( { SessionID: 1 } ) ) |

###options(\<options\>)###

对应 SQL 语法 [hint 子句][hint]中的 use_option 部分。

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec('select * from $SNAPSHOT_CONFIGS where GroupName = "db1" and ServiceName = "20000" /*+use_option(Mode, local) use_option(Expand, false)*/') | db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( { GroupName:'db1', ServiceName:'20000' } ).options( { "Mode": "local", "Expand": false } ) ) |

###skip(\<skip\>)###

对应 SQL 语法[ offset 子句][offset]

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( " select * from $SNAPSHOT_CONTEXT offset 2" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).skip( 2 ) ) |

###limit(\<limit\>)###

对应 SQL 语法 [limit 子句][limit]

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( "select * from $SNAPSHOT_CONTEXT limit 1" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).limit( 1 ) ) |

##SQL使用命令位置参数##

[命令位置参数][location]用于控制命令执行的位置信息。SQL 可以用 where 语句来使用命令位置参数。

###示例###

* 控制快照在指定节点运行

   ```lang-javascript
   > db.exec("select * from $SNAPSHOT_CONTEXT where Role = 'catalog'")
   {
     "NodeName": "hostname:30000",
     "SessionID": 21,
     "Contexts": [
       {
         "ContextID": 7764,
         "Type": "DUMP",
         "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
         "DataRead": 0,
         "IndexRead": 0,
         "QueryTimeSpent": 0,
         "StartTimestamp": "2019-06-26-17.55.42.355666"
       }
     ]
   }
   ...
   ```

* 控制快照不在全局执行

   ```lang-javascript
   > db.exec("select * from $SNAPSHOT_CONTEXT where Global = false")
   {
     "NodeName": "hostname:50000",
     "SessionID": 10,
     "Contexts": [
       {
         "ContextID": 70,
         "Type": "DUMP",
         "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
         "DataRead": 0,
         "IndexRead": 0,
         "QueryTimeSpent": 0,
         "StartTimestamp": "2019-06-26-17.56.55.916040"
       }
     ]
   }
   ```


[^_^]:
    本文使用的所有引用及链接
[SNAPSHOT_CONTEXT]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CONTEXT.md
[SDB_SNAP_CONTEXTS]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS.md
[SNAPSHOT_CONTEXT_CUR]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CONTEXT_CUR.md
[SDB_SNAP_CONTEXTS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS_CURRENT.md
[SNAPSHOT_SESSION]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_SESSION.md
[SDB_SNAP_SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[SNAPSHOT_SESSION_CUR]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_SESSION_CUR.md
[SDB_SNAP_SESSIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS_CURRENT.md
[SNAPSHOT_CL]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CL.md
[SDB_SNAP_COLLECTIONS]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONS.md
[SNAPSHOT_CS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CS.md
[SDB_SNAP_COLLECTIONSPACES]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONSPACES.md
[SNAPSHOT_DB]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_DB.md
[SDB_SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[SNAPSHOT_SYSTEM]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_SYSTEM.md
[DB_SNAP_SYSTEM]:manual/Manual/Snapshot/SDB_SNAP_SYSTEM.md
[SNAPSHOT_CATA]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CATA.md
[SDB_SNAP_CATALOG]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md
[SNAPSHOT_TRANS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_TRANS.md
[SDB_SNAP_TRANSACTIONS]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS.md
[SNAPSHOT_TRANS_CUR]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_TRANS_CUR.md
[SDB_SNAP_TRANSACTIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md
[SNAPSHOT_ACCESSPLANS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_ACCESSPLANS.md
[SDB_SNAP_ACCESSPLANS]:manual/Manual/Snapshot/SDB_SNAP_ACCESSPLANS.md
[SNAPSHOT_HEALTH]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_HEALTH.md
[SDB_SNAP_HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[SNAPSHOT_CONFIGS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CONFIGS.md
[SDB_SNAP_CONFIGS]:manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md
[SNAPSHOT_SEQUENCES]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_SEQUENCES.md
[SDB_SNAP_SEQUENCES]:manual/Manual/Snapshot/SDB_SNAP_SEQUENCES.md
[SNAPSHOT_SVCTASKS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_SVCTASKS.md
[SDB_SNAP_SVCTASKS]:manual/Manual/Snapshot/SDB_SNAP_SVCTASKS.md
[SNAPSHOT_QUERIES]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_QUERIES.md
[SDB_SNAP_QUERIES]:manual/Manual/Snapshot/SDB_SNAP_QUERIES.md
[SNAPSHOT_LOCKWAITS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_LOCKWAITS.md
[SDB_SNAP_LOCKWAITS]:manual/Manual/Snapshot/SDB_SNAP_LOCKWAITS.md
[SNAPSHOT_LATCHWAITS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_LATCHWAITS.md
[SDB_SNAP_LATCHWAITS]:manual/Manual/Snapshot/SDB_SNAP_LATCHWAITS.md
[SNAPSHOT_INDEXSTATS]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_INDEXSTATS.md
[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md
[SNAPSHOT_TRANSWAIT]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_TRANSWAITS.md
[SDB_SNAP_TRANSWAITS]:manual/Manual/Snapshot/SDB_SNAP_TRANSWAITS.md
[SNAPSHOT_TRANSDEADLOCK]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_TRANSDEADLOCK.md
[SDB_SNAP_TRANSDEADLOCK]:manual/Manual/Snapshot/SDB_SNAP_TRANSDEADLOCK.md
[SNAPSHOT_RECYCLEBIN]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_RECYCLEBIN.md
[SDB_SNAP_RECYCLEBIN]:manual/Manual/Snapshot/SDB_SNAP_RECYCLEBIN.md

[LIST_CONTEXT]:manual/Manual/SQL_Grammar/Monitoring/LIST_CONTEXT.md
[SDB_LIST_CONTEXTS]:manual/Manual/List/SDB_LIST_CONTEXTS.md
[LIST_CONTEXT_CUR]:manual/Manual/SQL_Grammar/Monitoring/LIST_CONTEXT_CUR.md
[SDB_LIST_CONTEXTS_CURRENT]:manual/Manual/List/SDB_LIST_CONTEXTS_CURRENT.md
[LIST_SESSION]:manual/Manual/SQL_Grammar/Monitoring/LIST_SESSION.md
[SDB_LIST_SESSIONS]:manual/Manual/List/SDB_LIST_SESSIONS.md
[LIST_SESSION_CUR]:manual/Manual/SQL_Grammar/Monitoring/LIST_SESSION_CUR.md
[SDB_LIST_SESSIONS_CURRENT]:manual/Manual/List/SDB_LIST_SESSIONS_CURRENT.md
[LIST_CL]:manual/Manual/SQL_Grammar/Monitoring/LIST_CL.md
[SDB_LIST_COLLECTIONS]:manual/Manual/List/SDB_LIST_COLLECTIONS.md
[LIST_CS]:manual/Manual/SQL_Grammar/Monitoring/LIST_CS.md
[SDB_LIST_COLLECTIONSPACES]:manual/Manual/List/SDB_LIST_COLLECTIONSPACES.md
[LIST_SU]:manual/Manual/SQL_Grammar/Monitoring/LIST_SU.md
[SDB_LIST_STORAGEUNITS]:manual/Manual/List/SDB_LIST_STORAGEUNITS.md
[LIST_GROUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_GROUP.md
[SDB_LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
[LIST_GROUPMODES]:manual/Manual/SQL_Grammar/Monitoring/LIST_GROUPMODES.md
[SDB_LIST_GROUPMODES]:manual/Manual/List/SDB_LIST_GROUPMODES.md
[LIST_TRANS]:manual/Manual/SQL_Grammar/Monitoring/LIST_TRANS.md
[SDB_LIST_TRANSACTIONS]:manual/Manual/List/SDB_LIST_TRANSACTIONS.md
[LIST_TRANS_CUR]:manual/Manual/SQL_Grammar/Monitoring/LIST_TRANS_CUR.md
[SDB_LIST_TRANSACTIONS_CURRENT]:manual/Manual/List/SDB_LIST_TRANSACTIONS_CURRENT.md
[LIST_SEQUENCES]:manual/Manual/SQL_Grammar/Monitoring/LIST_SEQUENCES.md
[SDB_LIST_SEQUENCES]:manual/Manual/List/SDB_LIST_SEQUENCES.md
[LIST_BACKUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_BACKUP.md
[SDB_LIST_BACKUPS]:manual/Manual/List/SDB_LIST_BACKUPS.md
[LIST_SVCTASKS]:manual/Manual/SQL_Grammar/Monitoring/LIST_SVCTASKS.md
[SDB_LIST_SVCTASKS]:manual/Manual/List/SDB_LIST_SVCTASKS.md
[LIST_USER]:manual/Manual/SQL_Grammar/Monitoring/LIST_USER.md
[SDB_LIST_USERS]:manual/Manual/List/SDB_LIST_USERS.md
[LIST_DATASOURCE]:manual/Manual/SQL_Grammar/Monitoring/LIST_DATASOURCE.md
[SDB_LIST_DATASOURCES]:manual/Manual/List/SDB_LIST_DATASOURCES.md
[LIST_RECYCLEBIN]:manual/Manual/SQL_Grammar/Monitoring/LIST_RECYCLEBIN.md
[SDB_LIST_RECYCLEBIN]:manual/Manual/List/SDB_LIST_RECYCLEBIN.md

[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
[SdbSnapshotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[where]:manual/Manual/SQL_Grammar/Clause/where.md
[order_by]:manual/Manual/SQL_Grammar/Clause/order_by.md
[hint]:manual/Manual/SQL_Grammar/Clause/hint.md
[offset]:manual/Manual/SQL_Grammar/Clause/offset.md
[limit]:manual/Manual/SQL_Grammar/Clause/limit.md
[location]:manual/Manual/Sequoiadb_Command/location.md
