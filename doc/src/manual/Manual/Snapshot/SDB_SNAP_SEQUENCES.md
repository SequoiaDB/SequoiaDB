[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190320
    评审意见

    王涛：
    许建辉：
    市场部：20190425



序列快照可以列出当前数据库中所有序列的属性信息。

> **Note:**  
>
> 序列快照只能在协调节点执行。

标识
----

SDB_SNAP_SEQUENCES

字段信息
----

| 字段名              | 类型   | 描述                         |
| ------------------- | ------ | ---------------------------- |
| Name                | string | 序列名称                     |
| ID                  | int64  | 序列 ID                      |
| Increment           | int32  | 序列增加的间隔               |
| StartValue          | int64  | 序列起始值                   |
| CurrentValue        | int64  | 序列当前值                   |
| MinValue            | int64  | 序列最小值                   |
| MaxValue            | int64  | 序列最大值                   |
| CacheSize           | int32  | 编目节点每次缓存序列值数     |
| AcquireSize         | int32  | 协调节点每次获取序列值数     |
| Cycled              | boolean| 序列值到达最大值（或最小值）是否允许循环 |
| CycledCount         | int32  | 序列已循环次数 |
| Version             | int32  | 序列版本号                   |
| Initial             | boolean| 该序列是否未使用，true 表示未使用 |
| Internal            | boolean| 该序列是否是系统内部序列     |

> **Note:**
>
> 序列未使用时，序列不存在当前值（CurrentValue）。使用前，CurrentValue 会显示为 StartValue，但该值无意义。

示例
----

查看序列快照

```lang-javascript
> db.snapshot( SDB_SNAP_SEQUENCES )
```

输出结果如下：

```lang-json
{
  "AcquireSize": 1000,
  "CacheSize": 1000,
  "CurrentValue": 5000,
  "Cycled": false,
  "CycledCount": 0,
  "ID": 4,
  "Increment": 10,
  "Initial": true,
  "Internal": true,
  "MaxValue": {
    "$numberLong": "9223372036854775807"
  },
  "MinValue": 1,
  "Name": "SYS_21333102559237_studentID_SEQ",
  "StartValue": 5000,
  "Version": 1,
  "_id": {
    "$oid": "5bd8fcfc8af29ca6ad2a32e8"
  }
}
```
