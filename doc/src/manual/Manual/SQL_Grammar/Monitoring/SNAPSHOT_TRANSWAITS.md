
事务等待快照可以列出数据库中因锁等待而产生的事务等待信息。如果连接协调节点执行，将返回所有主数据节点的当前事务等待信息；如果连接主数据节点，将返回该节点的当前事务等待信息。

> **Note：**
>
> 在输出结果中，每个事务等待信息为一条记录；如果节点未发生事务等待，则快照无输出信息。

##标识##

$SNAPSHOT_TRANSWAIT

##字段信息##

| 字段名                 | 类型      | 描述                                                            |
| ---------------------- | --------- | --------------------------------------------------------------- |
| NodeName               | string    | 锁等待所处节点对应的节点名，格式为<主机名>:<服务名>             |
| GroupID                | int32     | 锁等待所处节点对应的分区组 ID                                   |
| NodeID                 | int32     | 锁等待所处节点对应的节点 ID                                     |
| WaitTime               | int64     | 锁等待已耗费的时间，单位为毫秒                                  |
| WaiterTransID          | string    | 锁等待者事务 ID                                                 |
| HolderTransID          | string    | 锁持有者事务 ID                                                 |
| WaiterSessionID        | int64     | 锁等待者事务所处节点的会话 ID                                   |
| HolderSessionID        | int64     | 锁持有者事务所处节点的会话 ID                                   |
| WaiterTransCost        | int64     | 锁等待者事务在本节点的日志空间消耗                              |
| HolderTransCost        | int64     | 锁持有者事务在本节点的日志空间消耗                              |
| WaiterRelatedID        | string    | 锁等待者事务所在会话的内部标识                                  |
| HolderRelatedID        | string    | 锁持有者事务所在会话的内部标识                                  |
| WaiterRelatedGroupID   | int32     | 发起锁等待者事务的会话所处节点对应的分区组 ID                   |
| HolderRelatedGroupID   | int32     | 发起锁持有者事务的会话所处节点对应的分区组 ID                   |
| WaiterRelatedNodeID    | int32     | 发起锁等待者事务的会话所处节点对应的节点 ID                     |
| HolderRelatedNodeID    | int32     | 发起锁持有者事务的会话所处节点对应的节点 ID                     |
| WaiterRelatedSessionID | int64     | 发起锁等待者事务的会话所处节点对应的会话 ID                     |
| HolderRelatedSessionID | int64     | 发起锁持有者事务的会话所处节点对应的会话 ID                     |

##示例##

查看事务等待快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_TRANSWAIT" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname1:20000",
  "GroupID": 1000,
  "NodeID": 1000,
  "WaitTime": 8500,
  "WaiterTransID": "0x00040010ffeafb",
  "HolderTransID": "0x00040010ffeafd",
  "WaiterTransCost": 0,
  "HolderTransCost": 200,
  "WaiterSessionID": 32,
  "HolderSessionID": 28,
  "WaiterRelatedID": "7f000101c3500000000000000011",
  "HolderRelatedID": "7f000101c3500000000000000015",
  "WaiterRelatedSessionID": 17,
  "HolderRelatedSessionID": 21,
  "WaiterRelatedGroupID": 2,
  "HolderRelatedGroupID": 2,
  "WaiterRelatedNodeID": 4,
  "HolderRelatedNodeID": 4
}
{
  "NodeName": "hostname1:20000",
  "GroupID": 1000,
  "NodeID": 1000,
  "WaitTime": 6057,
  "WaiterTransID": "0x00040010ffeafd",
  "HolderTransID": "0x00040010ffeafa",
  "WaiterTransCost": 200,
  "HolderTransCost": 200,
  "WaiterSessionID": 28,
  "HolderSessionID": 25,
  "WaiterRelatedID": "7f000101c3500000000000000015",
  "HolderRelatedID": "7f000101c350000000000000000f",
  "WaiterRelatedSessionID": 21,
  "HolderRelatedSessionID": 15,
  "WaiterRelatedGroupID": 2,
  "HolderRelatedGroupID": 2,
  "WaiterRelatedNodeID": 4,
  "HolderRelatedNodeID": 4
}
{
  "NodeName": "hostname1:42000",
  "GroupID": 1001,
  "NodeID": 1003,
  "WaitTime": 10501,
  "WaiterTransID": "0x00040010ffeafa",
  "HolderTransID": "0x00040010ffeafb",
  "WaiterTransCost": 0,
  "HolderTransCost": 328,
  "WaiterSessionID": 33,
  "HolderSessionID": 26,
  "WaiterRelatedID": "7f000101c350000000000000000f",
  "HolderRelatedID": "7f000101c3500000000000000011",
  "WaiterRelatedSessionID": 15,
  "HolderRelatedSessionID": 17,
  "WaiterRelatedGroupID": 2,
  "HolderRelatedGroupID": 2,
  "WaiterRelatedNodeID": 4,
  "HolderRelatedNodeID": 4
}
{
  "NodeName": "hostname1:42000",
  "GroupID": 1001,
  "NodeID": 1003,
  "WaitTime": 7212,
  "WaiterTransID": "0x00040010ffeafc",
  "HolderTransID": "0x00040010ffeafb",
  "WaiterTransCost": 200,
  "HolderTransCost": 328,
  "WaiterSessionID": 29,
  "HolderSessionID": 26,
  "WaiterRelatedID": "7f000101c3500000000000000013",
  "HolderRelatedID": "7f000101c3500000000000000011",
  "WaiterRelatedSessionID": 19,
  "HolderRelatedSessionID": 17,
  "WaiterRelatedGroupID": 2,
  "HolderRelatedGroupID": 2,
  "WaiterRelatedNodeID": 4,
  "HolderRelatedNodeID": 4
}
{
  "NodeName": "hostname1:42000",
  "GroupID": 1001,
  "NodeID": 1003,
  "WaitTime": 6057,
  "WaiterTransID": "0x00040010ffeafd",
  "HolderTransID": "0x00040010ffeafc",
  "WaiterTransCost": 0,
  "HolderTransCost": 200,
  "WaiterSessionID": 34,
  "HolderSessionID": 29,
  "WaiterRelatedID": "7f000101c3500000000000000015",
  "HolderRelatedID": "7f000101c3500000000000000013",
  "WaiterRelatedSessionID": 21,
  "HolderRelatedSessionID": 19,
  "WaiterRelatedGroupID": 2,
  "HolderRelatedGroupID": 2,
  "WaiterRelatedNodeID": 4,
  "HolderRelatedNodeID": 4
}
Return 5 row(s).
```
