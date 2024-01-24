
[索引统计信息][statistics]快照可以列出当前数据库节点中所有的索引统计信息。

>**Note:**
>
> 每条索引为一条记录。

##标识##

$SNAPSHOT_INDEXSTATS

##字段信息##

| 字段名          | 数据类型 | 说明 |
| --------------- | -------- | ---- |
| NodeName        | string   | 集合所属节点名，格式为<主机名>:<端口号> |
| GroupName       | string   | 集合所属分区组名 |
| Collection      | string   | 统计的集合名称 |
| StatTimestamp   | string   | 统计收集的时间 |
| Index           | string   | 统计 Index 的名称 |
| TotalIndexLevels| int32    | 统计收集时索引的层数 |
| TotalIndexPages | int32    | 统计收集时索引的页个数 |
| Unique          | boolean  | Index 是否唯一索引 |
| KeyPattern      | object   | 统计索引的字段定义，例如：{a:1, b:-1} |
| DistinctValNum  | array    | 不重复的值的个数，抽样时，指样本中不重复值的个数<br>数组第 1 个元素表示字段定义中第 1 个字段的不重复值个数；第 2 个元素表示字段定义中第 1 和第 2 个字段的不重复值个数，以此类推<br>例如，字段定义为`{a:1, b:-1}`，数组为 [50, 100]，则 a 字段的不重复值有 50 个，a 和 b 字段组合的不重复值有 100 个|
| MinValue        | object   | 索引最小值，抽样时，指样本中的最小值 |
| MaxValue        | object   | 索引最大值，抽样时，指样本中的最大值 |
| NullFrac        | int32    | null 值的万分比，抽样时，指样本中 null 值的万分比 |
| UndefFrac       | int32    | undefined 值的万分比，抽样时，指样本中 undefined 值的万分比 |
| SampleRecords   | int64    | 统计收集时抽样的文档个数 |
| TotalRecords    | int64    | 统计收集时的文档个数 |

##示例##

查看索引统计信息快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_INDEXSTATS" )
```

输出结果如下

```lang-json
{
  "NodeName": "hostname:11840",
  "GroupName": "group2",
  "Collection": "sample.employees",
  "StatTimestamp": "2020-06-19-14.10.38.931000",
  "Index": "index01",
  "TotalIndexLevels": 2,
  "TotalIndexPages": 135,
  "Unique": false,
  "KeyPattern": {
    "activityType": 1,
    "status": 1
  },
  "DistinctValNum": [
    2,
    8
  ],
  "MinValue": {
    "activityType": 1,
    "status": 1
  },
  "MaxValue": {
    "activityType": 2,
    "status": 9
  },
  "NullFrac": 0,
  "UndefFrac": 0,
  "SampleRecords": 200,
  "TotalRecords": 136276
}
...
```


[^_^]:
    本文使用的所有引用及链接
[statistics]:manual/Distributed_Engine/Maintainance/Access_Plan/statistics.md
