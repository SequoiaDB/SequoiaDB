
闩锁等待快照可以列出数据库中正在进行的闩锁等待信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个闩锁等待为一条记录。

##标识##

$SNAPSHOT_LATCHWAITS

##字段信息##

| 字段名                 | 类型     | 描述                                                         |
| ---------------------- | -------- | ------------------------------------------------------------ |
| NodeName               | string   | 闩锁等待发生的所在节点名                                     |
| WaiterTID              | int32    | 等待闩锁的线程 ID                                             |
| RequiredMode           | string   | 上述线程要求获得的闩锁模式，分为 S 共享模式和 X 排他模式两种     |
| LatchName              | string   | 被等待闩锁对象的名称                                         |
| Address                | string   | 被等待闩锁对象的地址                                         |
| StartTimestamp         | string   | 本次等待开始时间                                             |
| LatchWaitTime          | int32    | 本次等待耗费时间，单位为毫秒                                 |
| LatestOwner            | int32    | 最近获得该闩锁的线程 ID                                       |
| LatestOwnerMode        | string   | 最近获得该闩锁线程所获得的模式，分为 S 共享模式和 X 排他模式两种 |
| NumOwner               | int32    | 该等待事件发生时被等待闩锁总共的持有者数量                   |

##示例##

查看闩锁等待快照

```lang-javascript
> db.exec( "select * from $SDB_SNAP_LATCHWAITS" )
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
以上述输出为例现在系统中正在发生一个闩锁等待事件，线程 24118 正在等待获取"dmsStorageDataCommon mblock"闩锁的 S 共享模式。到目前为止已经等待了 34.806 毫秒。线程 24109 是最近一个拿到这个闩锁的线程，拿到了 X 排他模式，此时闩锁只有 24109 一个拥有者。

