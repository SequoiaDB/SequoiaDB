[^_^]: 

    会话列表
    作者：何嘉文
    时间：20190522
    评审意见

    王涛：
    许建辉：
    市场部：



会话列表可以列出所有用户会话和系统会话。

##标识##

SDB_LIST_SESSIONS

##字段信息##

| 字段名    | 类型         | 描述                       |
| --------- | ------------ | -------------------------- |
| NodeName  | string       | 	会话所在的节点    |
| SessionID | int64        | 会话 ID                    |
| TID       | int32        | 该会话所对应的系统线程 ID  |
| Status    | string       | 会话状态，取值如下：<br>Creating：创建状态<br>Running：运行状态<br>Waiting：等待状态<br>Idle：线程池待机状态<br>Destroying：销毁状态 |
| Type      | string       | [EDU 类型][edu_url]        |
| Name      | string       | EDU 名，一般系统 EDU 名为空             |
| Source    | string       | 会话来源信息 |
| RelatedID | string       | 会话的内部标识             |

##示例##

查看会话列表

```lang-javascript
> db.list(SDB_LIST_SESSIONS)
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname1:11820",
  "SessionID": 1,
  "TID": 6168,
  "Status": "Running",
  "Type": "TCPListener",
  "Name": "",
  "Source": "MySQL:hostname1:32762:3",
  "RelatedID": "7f000101a4100000000000000001"
}
```

[^_^]:
    本文使用到的所有链接及引用。
    
[edu_url]: manual/Distributed_Engine/Architecture/Thread_Model/edu.md
