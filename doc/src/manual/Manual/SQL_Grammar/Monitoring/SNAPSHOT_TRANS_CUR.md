
当前事务快照可以列出当前会话在数据库中正在进行的事务信息，当前会话在每一个数据节点上正在进行的事务为一条记录。

> **Note:**
>
> 一般每个会话在每个数据节点上只有一个事务记录。

##标识##

$SNAPSHOT_TRANS_CUR

##字段信息##

字段说明可参考[当前事务快照][SDB_SNAP_TRANSACTIONS_CURRENT]。

##示例##

查看当前事务快照

```lang-javascript
> db.exec("select * from $SNAPSHOT_TRANS_CUR")
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:42000",
  "SessionID": 20,
  "TransactionID": "00040000000003",
  "TransactionIDSN": 3,
  "IsRollback": false,
  "CurrentTransLSN": 3314225876,
  "BeginTransLSN": 3314225744,
  "WaitLock": {},
  "TransactionLocksNum": 3,
  "IsLockEscalated": false,
  "UsedLogSpace": 100,
  "ReservedLogSpace": 116,
  "RelatedID": "c0a8143ec3500000000000000014",
  "GotLocks": [
    {
      "CSID": 906,
      "CLID": 0,
      "ExtentID": 9,
      "Offset": 128,
      "Mode": "X",
      "Count": 2,
      "Duration": 888435
    },
    {
      "CSID": 906,
      "CLID": 0,
      "ExtentID": -1,
      "Offset": -1,
      "Mode": "IX",
      "Count": 2,
      "Duration": 888436
    },
    {
      "CSID": 906,
      "CLID": 65535,
      "ExtentID": -1,
      "Offset": -1,
      "Mode": "IS",
      "Count": 2,
      "Duration": 888436
    }
  ]
}
```



[^_^]:
    本文使用的所有引用及链接
[SDB_SNAP_TRANSACTIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md
