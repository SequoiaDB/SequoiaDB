
事务快照可以列出数据库中正在进行的事务信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个事务为一条记录。

##标识##

$SNAPSHOT_TRANS

##字段信息##

字段说明可参考[事务快照][SDB_SNAP_TRANSACTIONS]。

##示例##

查看事务快照

```lang-javascript
> db.exec("select * from $SNAPSHOT_TRANS")
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
      "Duration": 907829
    },
    {
      "CSID": 906,
      "CLID": 0,
      "ExtentID": -1,
      "Offset": -1,
      "Mode": "IX",
      "Count": 2,
      "Duration": 907830
    },
    {
      "CSID": 906,
      "CLID": 65535,
      "ExtentID": -1,
      "Offset": -1,
      "Mode": "IS",
      "Count": 2,
      "Duration": 907830
    }
  ]
}
```


[^_^]:
    本文使用的所有引用及链接
[SDB_SNAP_TRANSACTIONS]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS.md
