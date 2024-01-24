在数据库中，死锁是两个或多个事务彼此等待对方所持有锁的情况。常见的死锁场景如下：

![常见死锁场景][sscDeadlock0]

图中 T1、T2 和 T3 分别表示三个事务，有向线段表示事务间的等待关系。事务 T1 指向事务 T2 表示 T1 正在等待 T2，以此类推，T2 在等待 T3，而 T3 在等待 T1，三个事务间构成了死锁。事务通常不会主动释放已持有的锁，因此死锁不会自动解除，造成资源浪费、事务处理停滞等情况。

数据库在运行过程中存在大量事务，以手工方式检测死锁是一项繁琐的工作。为此，SequoiaDB 巨杉数据库提供死锁检测功能，用于检测事务间是否存在死锁，并显示构成死锁的所有事务信息。用户可根据提供的事务信息终止相关的会话，以解除死锁。

在分布式环境中可能在每个数据节点都没有发生死锁，但在整个集群中却形成了死锁。因此通常在协调节点上执行死锁检测，便于对整个数据库系统进行检测。

##解除死锁步骤##

解除死锁步骤如下：

1. 对集群进行死锁检测，并输出相关的事务信息
2. 根据输出信息，终止事务所在的会话

###死锁检测###

用户可通过[事务死锁检测快照][SDB_SNAP_TRANSDEADLOCK]检测数据库中是否存在死锁。如果发现死锁，则输出每个死锁关联的事务信息；如果未发现死锁，则无输出。

假设集群中存在如下锁等待关系：

![关联死锁集][sscDeadlock1]

事务 T1（事务 ID 为"0x00040010ffeafc"）、T2（事务 ID 为"0x00040010ffeafb"）和 T3（事务 ID 为"0x00040010ffeafd"）形成死锁；事务 T2、T3 和 T4（事务 ID 为"0x00040010ffeafa"）形成另一个死锁。此时，这几个死锁将视为一个整体，称为关联死锁集。在关联死锁集中的任何两个事务间均存在等待与被等待关系。用户进行死锁检测时，将输出如下信息：

```lang-javascript
> db.snapshot(SDB_SNAP_TRANSDEADLOCK)
{
  "DeadlockID": 1,
  "TransactionID": "0x00040010ffeafd",
  "Degree": 3,
  "Cost": 200,
  "RelatedID": "7f000101c3500000000000000015",
  "SessionID": 21,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 1,
  "TransactionID": "0x00040010ffeafb",
  "Degree": 3,
  "Cost": 328,
  "RelatedID": "7f000101c3500000000000000011",
  "SessionID": 17,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 1,
  "TransactionID": "0x00040010ffeafa",
  "Degree": 2,
  "Cost": 200,
  "RelatedID": "7f000101c350000000000000000f",
  "SessionID": 15,
  "GroupID": 2,
  "NodeID": 4
}
{
  "DeadlockID": 1,
  "TransactionID": "0x00040010ffeafc",
  "Degree": 2,
  "Cost": 200,
  "RelatedID": "7f000101c3500000000000000013",
  "SessionID": 19,
  "GroupID": 2,
  "NodeID": 4
}
Return 4 row(s).
```

###终止相关会话###

在终止会话时，通常选择死锁中字段 Degree 值较大但字段 Cost 值较小的事务所在会话，实现以较小的代价解除死锁。
                                                 
以上述输出信息为例，应选择终止事务 T3 所在的会话。用户可通过 [forceSession()][forceSession] 终止该会话，以解除死锁。具体执行命令如下：

```lang-javascript
> db.forceSession(21, {"GroupID": 2, "NodeID": 4})
```

##注意事项##

在某些极端情形下，从每个死锁（或者每个关联死锁集）中挑选一个事务回滚，并不能解除全部死锁。如下图所示：

![极端死锁场景][sscDeadlock2]

图中只有一个关联死锁集，包含事务 T3、T4、T5、T8、T9 和 T10，此时终止任一事务均不能解除全部死锁。对于该极端情形，通常在终止第一个事务后，多次进行死锁检测，并终止相关的事务，直至死锁全部解除。


[^_^]:
     本文使用的所有引用及连接
[SDB_SNAP_TRANSDEADLOCK]:manual/Manual/Snapshot/SDB_SNAP_TRANSDEADLOCK.md
[SDB_SNAP_TRANSWAITS]:manual/Manual/Snapshot/SDB_SNAP_TRANSWAITS.md
[forceSession]:manual/Manual/Sequoiadb_Command/Sdb/forceSession.md
[sscDeadlock0]:images/Distributed_Engine/Maintainance/sscDeadlock0.png
[sscDeadlock1]:images/Distributed_Engine/Maintainance/sscDeadlock1.png
[sscDeadlock2]:images/Distributed_Engine/Maintainance/sscDeadlock2.png
