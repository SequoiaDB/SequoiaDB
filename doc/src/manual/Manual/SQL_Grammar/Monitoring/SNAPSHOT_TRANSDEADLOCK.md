
事务死锁检测快照可以列出数据库中处于死锁状态的事务信息。如果连接协调节点执行，将对整个数据库进行死锁检测；如果连接主数据节点执行，将只对该数据节点进行死锁检测。

> **Note：**
>
> 在输出结果中，构成死锁的每一个事务为一条记录；如果未发生死锁，则快照无输出信息。

##标识##

$SNAPSHOT_TRANSDEADLOCK

##字段信息##

| 字段名        | 类型       |     描述                                                                                   |
| --------------| -----------| -------------------------------------------------------------------------------------------|
| DeadlockID    | int32      | 死锁（或关联死锁集）在本次检测中的编号，编号相同的事务表示处于同一个死锁（或死锁关联集）中 |
| TransactionID | string     | 事务 ID   |
| Degree        | int32      | 事务在该死锁（或关联死锁集）中的关联度，即与该事务有等待或被等待关系的事务个数 |
| Cost          | int64      | 事务在该死锁（或关联死锁集）所包含的数据节点中使用的事务日志空间总量 |
| RelatedID     | string     | 事务所关联会话的内部标识    |
| SessionID     | int64      | 连接协调节点时，为发起该事务的会话 ID；连接数据节点时，为该事务在数据节点的会话 ID   |
| GroupID       | int32      | 连接协调节点时，为发起该事务的会话所处节点对应的分区组 ID；连接数据节点时，为该数据节点的分区组 ID   |
| NodeID        | int32      | 连接协调节点时，为发起该事务的会话所处节点对应的节点 ID；连接数据节点时，为该数据节点的节点 ID |

##示例##

检测当前数据库的事务间是否存在死锁

```lang-javascript
> db.exec( "select * from $SNAPSHOT_TRANSDEADLOCK" )
```

输出结果如下：

```lang-json
{
  "DeadlockID": 1,
  "TransactionID": "0x0004001122581f",
  "Degree": 3,
  "Cost": 200,
  "RelatedID": "7f000101c3500000000000000015",
  "SessionID": 21,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 1,
  "TransactionID": "0x0004001122581d",
  "Degree": 3,
  "Cost": 328,
  "RelatedID": "7f000101c3500000000000000011",
  "SessionID": 17,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 1,
  "TransactionID": "0x0004001122581c",
  "Degree": 2,
  "Cost": 200,
  "RelatedID": "7f000101c350000000000000000f",
  "SessionID": 15,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 1,
  "TransactionID": "0x0004001122581e",
  "Degree": 2,
  "Cost": 200,
  "RelatedID": "7f000101c3500000000000000013",
  "SessionID": 19,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 2,
  "TransactionID": "0x00040011225820",
  "Degree": 2,
  "Cost": 200,
  "RelatedID": "7f000101c3500000000000000017",
  "SessionID": 23,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 2,
  "TransactionID": "0x00040011225821",
  "Degree": 2,
  "Cost": 200,
  "RelatedID": "7f000101c3500000000000000019",
  "SessionID": 25,
  "GroupID": 2,
  "NodeID": 4
}
Return 6 row(s).
```

[^_^]:
    本文使用的所有引用及链接
[deadlock_detector]:manual/Distributed_Engine/Maintainance/deadlock_detector.md
[forceSession]:manual/Manual/Sequoiadb_Command/Sdb/forceSession.md
