
事务列表可以列出数据库中正在进行的事务信息。

> **Note:**
>
>每一个数据节点上正在进行的每一个事务为一条记录。

##标识##

$LIST_TRANS

##字段信息##

字段说明可参考[事务列表][SDB_LIST_TRANSACTIONS]。

##示例##

查看事务列表

```lang-javascript
> db.exec("select * from $LIST_TRANS")
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:42000",
  "GroupName": "db2",
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
  "RelatedID": "c0a8143ec3500000000000000014"
}
```



[^_^]:
    本文使用的所有引用及链接
[SDB_LIST_TRANSACTIONS]:manual/Manual/List/SDB_LIST_TRANSACTIONS.md
