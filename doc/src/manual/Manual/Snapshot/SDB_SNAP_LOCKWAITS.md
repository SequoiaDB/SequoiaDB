
锁等待快照可以列出数据库中正在发生的锁等待信息。当 [mongroupmask][configuration] 参数设置为“slowQuery:detail”或“all:detail”时，该次等待记录会在线程拿到该锁后被归入历史锁等待信息。用户可以通过指定 [viewHistory][SnapshotOption] 选项，查看历史锁等待信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个锁等待为一条记录。

##标识##

SDB_SNAP_LOCKWAITS 

##字段信息##

| 字段名                 | 类型     | 描述                                                           |
| ---------------------- | -------- | -------------------------------------------------------------- |
| NodeName               | string   | 锁等待发生的所在节点名                                         |
| WaiterTID              | int32    | 等待锁的线程 ID                                                |
| RequiredMode           | string   | 上述线程要求获得的锁模式，分为 S 共享模式和 X 排他模式两种     |
| CSID                   | int32    | 被等待锁对象所在集合空间的 ID                                  |
| CLID                   | int32    | 被等待锁对象所在集合的 ID                                      |
| ExtentID               | int32    | 被等待锁对象所在记录的 ID                                      |
| Offset                 | int32    | 被等待锁对象所在记录的偏移量                                   |
| StartTimestamp         | string   | 本次等待开始时间                                               |
| TransLockWaitTime      | int32    | 锁等待耗费时间，单位为毫秒                                     |
| LatestOwner            | int32    | 最近获得该锁的线程 ID                                          |
| LatestOwnerMode        | string   | 最近获得该锁的线程所获得的模式，分为 S 共享模式和 X 排他模式两种 |
| NumOwner               | int32    | 该等待事件发生时被等待闩锁总共的持有者数量                     |

##示例##

- 查看即时锁等待信息

   ```lang-javascript
   > db.snapshot(SDB_SNAP_LOCKWAITS)
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver:11870",
     "WaiterTID": 23853,
     "RequiredMode": "X",
     "CSID": 4,
     "CLID": 7,
     "ExtentID": 838,
     "Offset": 53396,
     "StartTimestamp": "2020-06-13-02.52.38.470191",
     "TransLockWaitTime": 18.815,
     "LatestOwner": 23532,
     "LatestOwnerMode": "X",
     "NumOwner": 1
   }
   ```

- 查看历史锁等待信息

   ```lang-javascript
   > db.snapshot(SDB_SNAP_LOCKWAITS, new SdbSnapshotOption().options({"viewHistory":true}))
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver:11870",
     "WaiterTID": 13602,
     "RequiredMode": "X",
     "CSID": 3,
     "CLID": 7,
     "ExtentID": 483,
     "Offset": 57688,
     "StartTimestamp": "2020-06-12-04.04.01.300151",
     "TransLockWaitTime": 14.05,
     "LatestOwner": 10307,
     "LatestOwnerMode": "X",
     "NumOwner": 1
   }
   ```



[^_^]:
    本文使用的所有引用及链接
[SnapshotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
