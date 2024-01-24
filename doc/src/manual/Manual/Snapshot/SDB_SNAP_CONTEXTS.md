[^_^]: 

    上下文快照
    作者：何嘉文
    时间：20190307
    评审意见

    王涛：
    许建辉：
    市场部：


上下文快照可以列出所有会话所对应的上下文。

> **Note:**  
> 快照操作会产生一个上下文，因此结果集中至少有当前会话的上下文。

标识
----

SDB_SNAP_CONTEXTS

字段信息
----

| 字段名    | 类型      | 描述                   |
| --------- | --------- | ---------------------- |
| NodeName  | string    | 节点名，格式为<主机名>:<服务名>|
| SessionID | int64     | 会话 ID                |
| Contexts.ContextID      | int64  | 上下文 ID                                                |
| Contexts.Type           | string | 上下文类型，如：DUMP、DATA、LIST_LOB、LOB、EXPLAIN、SORT |
| Contexts.Description    | string | 上下文的描述信息，如：当前的查询条件                     |
| Contexts.DataRead       | int64  | 所读数据                                                 |
| Contexts.IndexRead      | int64  | 所读索引                                                 |
| Contexts.LobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.LobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.LobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.LobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.QueryTimeSpent | double | 查询总时间，单位为秒                                     |
| Contexts.StartTimestamp | timestamp | 创建时间                                               |

示例
----

查看上下文快照

```lang-javascript
> db.snapshot( SDB_SNAP_CONTEXTS )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname1:11820",
  "SessionID": 32,
  "Contexts": [
    {
      "ContextID": 15,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "LobRead": 0,
      "LobWrite": 0,
      "LobTruncate": 0,
      "LobAddressing": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2022-10-06-18.37.08.926793"
    }
  ]
}
```