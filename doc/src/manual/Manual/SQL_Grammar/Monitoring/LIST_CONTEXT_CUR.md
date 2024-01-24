
当前上下文列表可以列出数据库节点中，当前连接所对应的会话中的上下文。

如果当前连接在协调节点上，将会返回当前会话通过协调节点连接各个数据节点或者编目节点的上下文，每个数据节点或者编目节点连接产生一条记录；如果当前连接在数据节点或者编目节点上，将会返回一条记录。每个记录中的 Contexts 数组字段中包含当前会话中所有的上下文。

>   **Note:**
>
>   列表操作自身需产生一个上下文，因此结果集中至少包含一个上下文。

##标识##

$LIST_CONTEXT_CUR

##字段信息##

| 字段名     | 类型       | 描述                                           |
| ---------- | ---------- | ---------------------------------------------- |
| NodeName   | string     | 上下文所在的节点                               |
| SessionID  | int64     | 会话 ID                                        |
| TotalCount | int32       | 上下文列表长度                                 |
| Contexts   | array     | 上下文 ID 数组，为该会话所包含的所有上下文列表 |

##示例##

查看当前上下文列表

```lang-javascript
> db.exec( "select * from $LIST_CONTEXT_CUR" )
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
