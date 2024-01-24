
会话列表可以列出当前数据库节点中所有的用户与系统会话，每一个会话为一条记录。

##标识##

$LIST_SESSION

##字段信息##

| 字段名    | 类型         | 描述                                                         |
| --------- | ------------ | ------------------------------------------------------------ |
| NodeName  | string       | 会话所在的节点                                               |
| SessionID | int32/int64 | 会话 ID                                                      |
| TID       | int32        | 该会话所对应的系统线程 ID                                    |
| Status    | string       | 会话状态，取值如下：<br> Creating：创建状态<br> Running：运行状态<br> Waiting：等待状态<br> Idle：线程池待机状态<br> Destroying：销毁状态 |
| Type      | string       | [EDU 类型](manual/Distributed_Engine/Architecture/Thread_Model/edu.md) |
| Name      | string        | EDU 名，一般系统 EDU 名为空                                  |
| Source            | string        | 会话来源信息，该字段仅在与 SQL 实例相关的会话中有值 |
| RelatedID | string        | 会话的内部标识                                               |

##示例##

查看会话列表

```lang-javascript
> db.exec( "select * from $LIST_SESSION" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:41000",
  "SessionID": 4,
  "TID": 23272,
  "Status": "Waiting",
  "Type": "DpsRollback",
  "Name": "",
  "Source": "",
  "RelatedID": "c0a8143ea0280000000000000004"
}
{
  "NodeName": "hostname:41000",
  "SessionID": 5,
  "TID": 23273,
  "Status": "Running",
  "Type": "Task",
  "Name": "PAGEMAPPING-JOB-D",
  "Source": "",
  "RelatedID": "c0a8143ea0280000000000000005"
}
...
```
