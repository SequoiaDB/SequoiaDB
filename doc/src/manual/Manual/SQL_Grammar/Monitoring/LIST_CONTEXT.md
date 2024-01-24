
上下文列表可以列出当前数据库节点中所有的会话所对应的上下文。

每一个会话为一条记录，如果一个会话中包括一个或一个以上的上下文时，其 Contexts 数组字段对每个上下文产生一个对象。

>   **Note:**
>
>   列表操作自身需产生一个上下文，因此结果集中至少会返回一个当前列表的上下文信息。

##标识##

$LIST_CONTEXT

##字段信息##

| 字段名     | 类型       | 描述                                           |
| ---------- | ---------- | ---------------------------------------------- |
| NodeName   | string     | 上下文所在的节点                               |
| SessionID  | int64      | 会话 ID                                        |
| TotalCount | int32      | 上下文列表长度                                 |
| Contexts   | array      | 上下文 ID 数组，为该会话所包含的所有上下文列表 |

##示例##

查看上下文列表

```lang-javascript
> db.exec( "select * from $LIST_CONTEXT" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:30000",
  "SessionID": 21,
  "TotalCount": 1,
  "Contexts": [
    143353
  ]
}
{
  "NodeName": "hostname:30010",
  "SessionID": 20,
  "TotalCount": 1,
  "Contexts": [
    13196
  ]
}
{
  "NodeName": "hostname:30020",
  "SessionID": 19,
  "TotalCount": 1,
  "Contexts": [
    13189
  ]
}
...
```
