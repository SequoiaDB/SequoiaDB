[^_^]:
    当前会话快照


当前会话快照可以列出当前用户的会话。

> **Note:**
>
> 如果连接协调节点查询快照，返回的是当前会话通过协调节点所连接的编目节点和数据节点的会话。

##标识##

SDB_SNAP_SESSIONS_CURRENT

##字段信息##

| 字段名            | 类型       | 描述                                               |
| ----------------- | ---------- | -------------------------------------------------- |
| NodeName          | string     | 节点名，格式为<主机名>:<服务名>                    |
| SessionID         | int64      | 会话 ID                                            |
| TID               | int32      | 该会话所对应的系统线程 ID                          |
| Status            | string     | 会话状态，取值如下：<br>"Creating"：创建状态 <br>"Running"：运行状态 <br>"Waiting"：等待状态  <br>"Idle"：线程池待机状态 <br>"Destroying"：销毁状态 |
| Type              | string     | [EDU 类型][edu_url]                                |
| IsBlocked         | boolean    | 会话当前是否处理阻塞状态                           |
| Name              | string     | EDU 名，一般系统 EDU 名为空                                             |
| Doing             | string     | 会话当前阻塞状态的详细描述信息                     |
| Source            | string     | 会话来源信息                                       |
| QueueSize         | int32      | 等待处理请求的队列长度                             |
| ProcessEventCount | int64      | 已经处理请求的数量                                 |
| MemPoolSize       | 长整型        | Pool Memory 的大小，单位为字节                   |
| RelatedID         | string     | 会话的内部标识                                     |
| Contexts          | bson array | 该会话所有上下文的 ID                              |
| TotalQuery        | int64      | 总查询数量（广义查询，泛指在数据库上执行的所有操作）  |
| TotalSlowQuery    | int64      | 总慢查询数量（广义查询，泛指在数据库上执行的所有操作）|
| TotalTransCommit  | int64      | 总事务提交数量                                     |
| TotalTransRollback| int64      | 总事务回滚数量                                     |
| TotalDataRead     | int64      | 数据记录读                                         |
| TotalIndexRead    | int64      | 索引读                                             |
| TotalDataWrite    | int64      | 数据记录写                                         |
| TotalIndexWrite   | int64      | 索引写                                             |
| TotalUpdate       | int64      | 总更新记录数量                                     |
| TotalDelete       | int64      | 总删除记录数量                                     |
| TotalInsert       | int64      | 总插入记录数量                                     |
| TotalSelect       | int64      | 总选取记录数量                                     |
| TotalRead         | int64      | 总数据读                                           |
| TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| TotalReadTime     | int64      | 总数据读时间，单位为毫秒                           |
| TotalWriteTime    | int64      | 总数据写时间，单位为毫秒                           |
| ReadTimeSpent     | int64      | 读取记录的时间，单位为毫秒                         |
| WriteTimeSpent    | int64      | 写入记录的时间，单位为毫秒                         |
| ConnectTimestamp  | timestamp  | 连接发起时间                                       |
| ResetTimestamp    | timestamp  | 重置快照的时间                                     |
| LastOpType        | string     | 最后一次操作的类型，如：INSERT、UPDATE、COMMAND、GETMORE |
| LastOpBegin       | string     | 最后一次操作的起始时间                             |
| LastOpEnd         | string     | 最后一次操作的结束时间                             |
| LastOpInfo        | string     | 最后一次操作的详细信息                             |
| UserCPU           | double     | 用户 CPU，单位为秒                                 |
| SysCPU            | double     | 系统 CPU，单位为秒                                 |

##示例##

查看当前会话快照

```lang-javascript
> db.snapshot(SDB_SNAP_SESSIONS_CURRENT)
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:11810",
  "SessionID": 2,
  "TID": 14593,
  "Status": "Running",
  "IsBlocked": false,
  "Type": "CoordMgr",
  "Name": "",
  "Doing": "",
  "Source": "",
  "QueueSize": 0,
  "ProcessEventCount": 11,
  "MemPoolSize": 4192,
  "RelatedID": "c0a81e442e72000000000000001c",
  "Contexts": [
    38
  ],
  "TotalQuery": 1,
  "TotalSlowQuery": 0,
  "TotalTransCommit": 0,
  "TotalTransRollback": 0,
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 0,
  "TotalRead": 0,
  "TotalLobGet": 0,
  "TotalLobPut": 0,
  "TotalLobDelete": 0,
  "TotalLobList": 0,
  "TotalLobReadSize": 0,
  "TotalLobWriteSize": 0,
  "TotalLobRead": 0,
  "TotalLobWrite": 0,
  "TotalLobTruncate": 0,
  "TotalLobAddressing": 0,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ReadTimeSpent": 0,
  "WriteTimeSpent": 0,
  "ConnectTimestamp": "2022-10-06-18.00.06.717121",
  "ResetTimestamp": "2022-10-06-18.00.06.717121",
  "LastOpType": "COMMAND",
  "LastOpBegin": "2022-10-06-18.32.58.692587",
  "LastOpEnd": "--",
  "LastOpInfo": "",
  "UserCPU": 0.02,
  "SysCPU": 0.01
}
```

[^_^]:
    本文使用到的所有链接及引用
[edu_url]: manual/Distributed_Engine/Architecture/Thread_Model/edu.md
