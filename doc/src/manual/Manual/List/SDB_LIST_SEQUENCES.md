[^_^]: 

    序列列表
    作者：何嘉文
    时间：20190523
    评审意见

    王涛：
    许建辉：
    市场部：


序列列表可以列出所有序列信息。

> **Note:**  
> 该列表只支持在协调节点上使用。

标识
----

SDB_LIST_SEQUENCES

字段信息
----

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| Name           | string | 序列名                      |

示例
----

查看序列列表

```lang-javascript
> db.list(SDB_LIST_SEQUENCES)
```

输出结果如下：

```lang-json
{
  "Name": "SYS_21333102559237_studentID_SEQ"
}
{
  "Name": "SYS_21333102559238_teacherID_SEQ"
}
...
```
