
当前会话列表可以列出数据库节点中的当前用户会话。

如果当前连接在协调节点上，将会返回当前会话通过协调节点连接各个数据节点或者编目节点的会话，每个数据节点或者编目节点连接产生一条记录；如果当前连接在数据节点或者编目节点上，将会返回一条记录。

##标识##

$LIST_SESSION_CUR

##字段信息##

| 字段名    | 类型         | 描述                                                         |
| --------- | ------------ | ------------------------------------------------------------ |
| NodeName  | string       | 会话所在的节点                                               |
| SessionID | int32/int64 | 会话 ID                                                      |
| TID       | int32         | 该会话所对应的系统线程 ID                                    |
| Status    | int32       | 会话状态，取值如下：<br> Creating：创建状态<br> Running：运行状态<br> Waiting：等待状态<br> Idle：线程池待机状态<br> Destroying：销毁状态 |
| Type      | int32       | [EDU 类型](manual/Distributed_Engine/Architecture/Thread_Model/edu.md) |
| Name      | int32       | EDU 名，一般系统 EDU 名为空                                  |
| Source            | int32        | 会话来源信息，该字段仅在与 SQL 实例相关的会话中有值 |
| RelatedID | int32       | 会话的内部标识                                               |

##示例##

查看当前会话列表

```lang-javascript
> db.exec( "select * from $LIST_SESSION_CUR" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:30000",
  "SessionID": 29,
  "TID": 22121,
  "Status": "Running",
  "Type": "ShardAgent",
  "Name": "Type:Shard,NetID:5,R-TID:24371,R-IP:192.168.20.62,R-Port:50000",
  "Source": "",
  "RelatedID": "c0a8143ec350000000000000001d"
}
{
  "NodeName": "hostname:30010",
  "SessionID": 25,
  "TID": 22596,
  "Status": "Running",
  "Type": "ShardAgent",
  "Name": "Type:Shard,NetID:1,R-TID:24371,R-IP:192.168.20.62,R-Port:50000",
  "Source": "",
  "RelatedID": "c0a8143ec3500000000000000019"
}
...
```
