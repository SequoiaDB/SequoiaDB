

闩锁等待快照可以列出数据库中正在发生的闩锁等待信息。当 [mongroupmask][configuration] 参数设置为“slowQuery:detail”或“all:detail”时，该次等待记录会在线程拿到该闩锁后被归入历史闩锁等待信息。用户可以通过指定 [viewHistory][SnapshotOption] 选项，查看历史闩锁等待信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个闩锁等待为一条记录。


##标识##

SDB_SNAP_LATCHWAITS

##字段信息##

| 字段名                 | 类型     | 描述                                                         |
| ---------------------- | -------- | ------------------------------------------------------------ |
| NodeName               | string   | 闩锁等待发生的所在节点名                                     |
| WaiterTID              | int32    | 等待闩锁的线程 ID                                            |
| RequiredMode           | string   | 上述线程要求获得的闩锁模式，分为 S 共享模式和 X 排他模式两种 |
| LatchName              | string   | 被等待闩锁对象的名称                                         |
| Address                | string   | 被等待闩锁对象的地址                                         |
| StartTimestamp         | string   | 本次等待开始时间                                             |
| LatchWaitTime          | int32    | 本次等待耗费时间，单位为毫秒                                 |
| LatestOwner            | int32    | 最近获得该闩锁的线程 ID                                      |
| LatestOwnerMode        | string   | 最近获得该闩锁线程所获得的模式，分为 S 共享模式和 X 排他模式两种 |
| NumOwner               | int32    | 该等待事件发生时，被等待闩锁总共的持有者数量                   |


##示例##

- 查看即时闩锁等待信息

   ```lang-javascript
   > db.snapshot(SDB_SNAP_LATCHWAITS)
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver:11870",
     "WaiterTID": 24118,
     "RequiredMode": "S",
     "LatchName": "dmsStorageDataCommon mblock",
     "Address": "0x7fd3740ae410",
     "StartTimestamp": "2020-06-13-02.52.50.336383",
     "LatchWaitTime": 34.806,
     "LatestOwner": 24109,
     "LatestOwnerMode": "X",
     "NumOwner": 1
   }
   ```

   上述输出显示此时系统中正在发生一个闩锁等待事件，线程 24118 正在等待获取"dmsStorageDataCommon mblock"闩锁的 S 共享模式，本次等待到目前为止已耗时 34.806 毫秒。"LatestOwner"显示最近获得该闩锁的线程为 24109，"LatestOwnerMode"显示上一次拿到的闩锁为 X 排他模式，"NumOwner"显示闩锁只有 24109 一个拥有者。

- 查看历史闩锁等待信息

   ```lang-javascript
   > db.snapshot(SDB_SNAP_LATCHWAITS, new SdbSnapshotOption().options({"viewHistory":true}))
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver:11870",
     "WaiterTID": 9726,
     "RequiredMode": "X",
     "LatchName": "dpsTransLockManager rwMutex",
     "Address": "0x7f8dca267d40",
     "StartTimestamp": "2020-06-12-04.02.22.096686",
     "LatchWaitTime": 1.172,
     "LatestOwner": 13608,
     "LatestOwnerMode": "S",
     "NumOwner": 1
   }
   ```

   上述输出显示系统中之前发生了一次等待事件，线程 9726 从"2020-06-12-04.02.22.096686"开始等待，最终获取"dpsTransLockManager rwMutex"闩锁的 X 排他模式，本次等待耗时 1.172 毫秒。


[^_^]:
    本文使用的所有引用及链接
[SnapshotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md