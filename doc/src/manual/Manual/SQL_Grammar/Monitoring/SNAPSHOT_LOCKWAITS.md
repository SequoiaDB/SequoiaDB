
锁等待快照可以列出数据库中正在等待的锁信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个锁等待为一条记录。

##标识##

$SNAPSHOT_LOCKWAITS

##字段信息##

| 字段名                 | 类型     | 描述                                                           |
| ---------------------- | -------- | -------------------------------------------------------------- |
| NodeName               | string   | 锁等待发生的所在节点名                                         |
| WaiterTID              | int32    | 等待锁的线程 ID                                                 |
| RequiredMode           | string   | 上述线程要求获得的锁模式，分为 S 共享模式和 X 排他模式两种         |
| CSID                   | int32    | 被等待锁对象所在集合空间的 ID                                  |
| CLID                   | int32    | 被等待锁对象所在集合的 ID                                      |
| ExtentID               | int32    | 被等待锁对象所在记录的 ID                                      |
| Offset                 | int32    | 被等待锁对象所在记录的偏移量                                   |
| StartTimestamp         | string   | 本次等待开始时间                                               |
| TransLockWaitTime      | int32    | 锁等待耗费时间，单位为毫秒                                     |
| LatestOwner            | int32    | 最近获得该锁的线程 ID                                           |
| LatestOwnerMode        | string   | 最近获得该锁的线程所获得的模式，分为 S 共享模式和 X 排他模式两种   |
| NumOwner               | int32    | 该等待事件发生时被等待闩锁总共的持有者数量                     |

##示例##

查看锁等待快照

```
> db.exec( "select * from $SDB_SNAP_LOCKWAITS" )
```

输出结果如下：

```lang-json
{
  "NodeName": "yang-VirtualBox:11870",
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
