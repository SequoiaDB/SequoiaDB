[^_^]:
    统计信息
    作者：刘晓旋
    时间：20190228
    修改时间：2019/11/22
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20191122


SequoiaDB 巨杉数据库提供收集统计信息的能力。外部收集而来的统计信息用于分析处理，分析后的最优结果将会被用于生成最优的访问计划，以此提高查询效率。因此统计信息对查询访问计划的构成以及查询性能起关键性作用。
SequoiaDB 包含两种统计信息，分别是集合的统计信息和索引的统计信息。

集合的统计信息
----

集合的统计信息存放在数据节点 SYSSTAT.SYSCOLLECTIONSTAT 集合中，具体字段如下：

| 字段名          | 数据类型   | 说明                                 | 必填 |
| --------------- | ---------- | ------------------------------------ | ---- |
| CollectionSpace | string     | 统计收集的集合空间名                 | 是   |
| Collection      | string     | 统计收集的集合名                     | 是   |
| CreateTime      | number     | 统计收集的时间戳，默认值为 0         | 是   |
| SampleRecords   | number     | 统计收集时抽样的文档个数，默认值为 0 | 是   |
| TotalDataPages  | number     | 统计收集时的数据页个数，默认值为 1   | 是   |
| TotalDataSize   | number     | 统计收集时的数据总大小（字节数）     | 是   |
| TotalRecords    | number     | 统计收集时的文档个数，默认值为 10    | 是   |
| AvgNumFields    | number     | 文档的平均字段数，默认值为 10        | 否   |

**示例**

SYSSTAT.SYSCOLLECTIONSTAT 统计信息

```lang-json
{
  "Collection": "sample",
  "CollectionSpace": "employee",
  "CreateTime": 1496910925978,
  "SampleRecords": 200,
  "TotalDataPages": 1284,
  "TotalDataSize": 65929411,
  "TotalRecords": 600000,
  "AvgNumFields" : 10
}
```

索引的统计信息
----

索引的统计信息存放在数据节点 SYSSTAT.SYSINDEXSTAT 集合中，具体字段如下：

| 字段名          | 数据类型   | 说明                                                         | 必填 |
| --------------- | ---------- | ------------------------------------------------------------ | ---- |
| CollectionSpace | string     | 统计收集的集合空间名                                         | 是   |
| Collection      | string     | 统计收集的集合名                                             | 是   |
| CreateTime      | number     | 统计收集的时间戳，默认值为 0                                 | 是   |
| Index           | string     | 统计收集的索引名                                             | 是   |
| IndexLevels     | number     | 统计收集时索引的层数，默认值为 1                             | 是   |
| IndexPages      | number     | 统计收集时索引页的个数，默认值为 1                           | 是   |
| IsUnique        | boolean    | 统计收集的索引是否唯一索引，默认值为 false                   | 是   |
| KeyPattern      | object     | 统计收集的索引字段定义，如：{a:1}                            | 是   |
| SampleRecords   | number     | 统计收集时抽样的文档个数，默认值为 0                         | 是   |
| TotalRecords    | number     | 统计收集时的文档个数，默认值为 10                            | 是   |
| MCV             | object     | 频繁数值集合（Most Common Values），如：<br/>MCV: { Values: [ {a:1}, {a:2}, ... ], Frac: [ 50, 50, ... ] } | 否   | 
| MCV.Values      | array      | 频繁数值中的值                                                | 否   |
| MCV.Frac        | array      | 频繁数值的比例，每个值的取值 0~10000，最终比例为 (Frac / 10000) * 100% | 否   |

**示例**

SYSSTAT.SYSINDEXSTAT 统计信息

```lang-json
{
  "Collection": "sample",
  "CollectionSpace": "employee",
  "CreateTime": 1496910926035,
  "Index": "index",
  "IndexLevels": 2,
  "IndexPages": 256,
  "IsUnique": false,
  "KeyPattern": {
    "a": 1
  },
  "MCV": {
    "Values": [
      {
        "a": 2358
      },
      {
        "a": 7074
      },
      {
        "a": 11790
      },
      ...
    ],
    "Frac": [
      50,
      50,
      50,
      ...
    ]
  },
  "SampleRecords": 200,
  "TotalRecords": 600000
}
```

评估策略
----

查询优化器会根据统计信息对候选访问计划进行评估，以此选取合适的访问计划来执行查询，详情见[基于代价的访问计划评估][cost_estimation]。

### 相等比较的选择率估算
- 如果字段建立了唯一索引，则选择率为：selectivity=1/TotalRecords
- 如果相等比较的值落入频繁数值集合中，假设命中下标为 i，则选择率为：selectivity=MCV.Frac[i]
- 如果相等比较的值没有落入频繁数值集合中，则选择率为：selectivity=(1-sum(MCV.Frac))*0.005

### 范围比较的选择率估算

- 如果相等比较的范围落入频繁数值集合中，假设命中下标为 m 至 n，则选择率为：selectivity=MCV.Frac[m]+...+MCV.Frac[n]
- 如果相等比较的范围没有落入频繁数值集合中，则选择率为：selectivity=(1-sum(MCV.Frac))*0.05

### 示例

如表中字段 val 建立了索引，生成该表的统计信息，字段  val 的频繁数值集合为：

```lang-json
MCV : {
  Val : [
    1, 2, 3, 4, 5, 6, 7, 8, 9
  ],
  Frac : [
    1000, 1200, 800, 1300, 700, 1000, 1000, 1000, 1000
  ]
}
```

- 如果字段索引是唯一索引，相等条件选择率为：selectivity = 1/TotalRecords
- 如果字段索引是普通索引，相等条件 `{ val : { $et : 1 } }` 命中频繁数值集合，其选择率为：selectivity= 0.1
- 如果字段索引是普通索引，相等条件 `{ val : { $et : 10 } }` 没有命中频繁数值集合，其选择率为：selectivity=0.0005
- 范围条件 `{ val : { $lt : 4 } }` 命中了频繁数值集合的下标 0、1 和 2，其选择率为：selectivity=0.2 
- 范围条件 `{ val : { $gt : 9 } }` 没有命中频繁数值集合，其选择率为：selectivity=0.005


[^_^]:
     本文使用的所有引用和链接
[cost_estimation]:manual/Distributed_Engine/Maintainance/Access_Plan/cost_estimation.md
