[^_^]: 

    集合列表
    作者：何嘉文
    时间：20190522
    评审意见
    
    王涛：
    许建辉：
    市场部：

集合列表：

- 连接非协调节点，列出所有集合（不含临时的集合）
- 连接协调节点，列出所有集合（不含临时的集合和系统的集合）

标识
----

SDB_LIST_COLLECTIONS

字段信息
----

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Name   | string | 集合完整名 |

示例
----

查看集合列表

```lang-javascript
> db.list( SDB_LIST_COLLECTIONS )
```

输出结果如下：

```lang-json
{
  "Name": "sample.employee"
}
...
```
