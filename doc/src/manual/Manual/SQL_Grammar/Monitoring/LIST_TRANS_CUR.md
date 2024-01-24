
当前事务列表可以列出当前会话在数据库中正在进行的事务信息，当前会话在每一个数据节点上正在进行的事务为一条记录。

>**Note:**
>
> 一般每个会话在每个数据节点上只有一个事务记录。

##标识##

$LIST_TRANS_CUR

##字段信息##

字段说明可参考[当前事务列表][SDB_LIST_TRANSACTIONS_CURRENT]。

##示例##

查看当前事务列表

```lang-javascript
> db.exec("select * from $LIST_TRANS_CUR")
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
  "CurrentTransLSN": 3314225744,
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
[SDB_LIST_TRANSACTIONS_CURRENT]:manual/Manual/List/SDB_LIST_TRANSACTIONS_CURRENT.md
