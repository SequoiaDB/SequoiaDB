[^_^]: 

    当前会话列表
    作者：何嘉文
    时间：20190522
    评审意见

    王涛：
    许建辉：
    市场部：


当前会话列表可以列出当前用户的会话。  
- 连接协调节点，返回协调节点所连接的编目节点和数据节点的会话
- 连接编目节点或者数据节点，返回当前会话

##标识##

SDB_LIST_SESSIONS_CURRENT

##字段信息##

| 字段名    | 类型         | 描述                                   |
| --------- | ------------ | -------------------------------------- |
| NodeName  | string       | 会话所在的节点        |
| SessionID | int64        | 会话 ID                                |
| TID       | int32        | 该会话所对应的系统线程 ID              |
| Status    | string       | 会话状态，取值如下<br>Creating：创建状态<br>Running：运行状态<br>Waiting：等待状态<br>Idle：线程池待机状态<br>Destroying：销毁状态 |
| Type      | string       | [EDU 类型][edu]                    |
| Name      | string       | EDU 名一般系统 EDU 名为空                                 |
| Source    | string       | 会话来源信息 |
| RelatedID | string       | 会话的内部标识                         |

##示例##

查看当前会话列表

```lang-javascript
> db.list(SDB_LIST_SESSIONS_CURRENT)  
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname1:11820",
  "SessionID": 21,
  "TID": 6691,
  "Status": "Running",
  "Type": "ShardAgent",
  "Name": "hostname1:11821",
  "Source": "",
  "RelatedID": "7f0001019c400000000000000015"
}
```

[^_^]:
    本文使用到的所有链接及引用。
    
[edu]:manual/Distributed_Engine/Architecture/Thread_Model/edu.md
