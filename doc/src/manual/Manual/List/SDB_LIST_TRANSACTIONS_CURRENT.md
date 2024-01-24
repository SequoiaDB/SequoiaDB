[^_^]: 
    当前事务列表

当前事务列表可以列出当前会话正在进行的事务信息。

> **Note:**  
>
> 事务功能可以参考[事务][transaction_url]。

标识
----

SDB_LIST_TRANSACTIONS_CURRENT

字段信息
----

| 字段名                 | 类型     | 描述                                     |
| ---------------------- | -------- | ---------------------------------------- |
| NodeName               | string   | 节点名格式为<主机名>:<端口号>            |
| GroupName              | string   | 数据组名                                 |
| SessionID              | int64    | 会话 ID                                  |
| TransactionID          | string   | 事务 ID                                  |
| IsRollback             | boolean  | 表示这个事务是否处于回滚中               |
| CurrentTransLSN        | int64    | 事务当前的日志 LSN                       |   
| WaitLock               | bson     | 正在等待的锁                             |
| TransactionLocksNum    | int32    | 事务已经获得的锁                         |
| IsLockEscalated        | boolean  | 事务是否已触发锁升级                     |
| UsedLogSpace           | int64    | 事务已使用的日志空间，单位为字节         |
| ReservedLogSpace       | int64    | 事务为回滚操作保留的日志空间，单位为字节 |
| RelatedID              | string   | 内部标识                                 |

> **Note:**  
>
> 当 WaitLock 没有锁对象时，表示事务没有在等待锁。

###锁对象信息###

WaitLock 字段中锁对象的信息如下：

| 字段名       | 类型    | 描述                     |
| ------------ | ------  | ------------------------ |
| CSID         | int32   | 锁对象所在集合空间的 ID  |
| CLID         | int32   | 锁对象所在集合的 ID      |
| ExtentID     | int32   | 锁对象所在记录的 ID      |
| Offset       | int32   | 锁对象所在记录的偏移量   |
| Mode         | string  | 锁的类型：<br>IS：意向共享锁<br>IX：意向排他锁<br>S：共享锁<br>U：升级锁<br>X：排他锁 |
| Duration     | int32   | 锁的持有或等待时间，单位为毫秒 |

###锁对象的描述###

锁对象每个字段取值不同表示不同的锁对象：

| 锁对象       | CSID | CLID  | ExtentID | Offset | 备注 |
| ------------ | ---- | ----- | ---- | ---- | ------------ |
| 没有锁对象   | -1   | 65535 | -1   | -1   | 一般在 WaitLock 为没有锁对象时，表示当前事务没有在等待锁 |
| 集合空间锁   | >= 0 | 65535 | -1   | -1   | |
| 集合锁       | >= 0 | >= 0  | -1   | -1   | |
| 记录锁       | >= 0 | >= 0  | >= 0 | >= 0 | |

示例
---

查看当前事务列表

```lang-javascript
> db.list(SDB_LIST_TRANSACTIONS_CURRENT)
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:20000",
  "GroupName": "db1",
  "SessionID": 89,
  "TransactionID": "03e80000000001",
  "IsRollback": false,
  "CurrentTransLSN": -1,
  "WaitLock": {},
  "TransactionLocksNum": 3,
  "IsLockEscalated": false,
  "UsedLogSpace": 100,
  "ReservedLogSpace": 116,
  "RelatedID": "c0a81457c3500000000000000059"
}
```

[^_^]:
    本文使用到的所有链接及引用。
    
[transaction_url]:manual/Distributed_Engine/Architecture/Transactions/Readme.md
