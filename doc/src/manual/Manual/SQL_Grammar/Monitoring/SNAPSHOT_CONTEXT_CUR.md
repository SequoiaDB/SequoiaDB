
当前上下文快照可以列出数据库节点中，当前连接所对应的会话中的上下文。

如果当前连接在协调节点上，将会返回当前会话通过协调节点连接各个数据节点或者编目节点的上下文，每个数据节点或者编目节点连接产生一条记录；如果当前连接在数据节点或者编目节点上，将会返回一条记录。每个记录中的 Contexts 数组字段中包含当前会话中所有的上下文。

>   **Note:** 
>
>   快照操作自身需产生一个上下文，因此结果集中至少包含一个上下文。

##标识##

$SNAPSHOT_CONTEXT_CUR

##字段信息##
 
| 字段名                    | 类型     | 描述                                     |
| ------------------------- | -------- | ---------------------------------------- |
| NodeName                  | string   | 节点名，格式为<主机名>:<端口号>                 |
| SessionID                 | int64    | 会话 ID                                  |
| Contexts.ContextID        | int64    | 上下文 ID                                |
| Contexts.Type             | string   | 上下文类型，如 DUMP                     |
| Contexts.Description      | string   | 上下文的描述信息，如：包含当前的查询条件 |
| Contexts.DataRead         | int64    | 所读数据                                 |
| Contexts.IndexRead        | int64    | 所读索引                                 |
| Contexts.LobRead        | int64  | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.LobWrite       | int64  | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.LobTruncate    | int64  | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.LobAddressing  | int64  | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| Contexts.QueryTimeSpent   | double   | 查询总时间，单位为秒                   |
| Contexts.StartTimestamp   | timestamp   | 创建时间                                 |

##示例##

查看当前上下文快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CONTEXT_CUR" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:30000",
  "SessionID": 29,
  "Contexts": [
    {
      "ContextID": 14,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "LobRead": 0,
      "LobWrite": 0,
      "LobTruncate": 0,
      "LobAddressing": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2022-10-09-10.39.11.912364"

    }
  ]
}
{
  "NodeName": "hostname:30010",
  "SessionID": 25,
  "Contexts": [
    {
      "ContextID": 14,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "LobRead": 0,
      "LobWrite": 0,
      "LobTruncate": 0,
      "LobAddressing": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2022-10-09-10.39.11.912395"
    }
  ]
}
...
```
