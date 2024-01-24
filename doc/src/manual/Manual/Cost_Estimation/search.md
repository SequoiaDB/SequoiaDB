

用户可以使用 [SdbQuery.explain\(\)][explain] 查看访问计划的搜索过程信息。

当 SdbQuery.explain() 的 Search 选项为 true 时，将展示以下信息：

| 字段名                         | 类型      | 描述                                                                                 |
| ------------------------------ | --------- | ------------------------------------------------------------------------------------ |
| Constants                      | bson      | 生成访问计划使用的常量<br>该字段在 Evaluate 选项为 true 时显示                                |
| Constants.RandomReadIOCostUnit | int32     | 随机读取 IO 的代价，默认值为 10                                                      |
| Constants.SeqReadIOCostUnit    | int32     | 顺序读取 IO 的代价，默认值为 1                                                       |
| Constants.SeqWrtIOCostUnit     | int32     | 顺序写入 IO 的代价，默认值为 2                                                       |
| Constants.PageUnit             | int32     | 数据页的单位，默认值为 4096 （单位：字节）                                           |
| Constants.RecExtractCPUCost    | int32     | 从数据页中提取数据的 CPU 代价，默认值为 4                                            |
| Constants.IXExtractCPUCost     | int32     | 从索引页中提取索引项的 CPU 代价，默认值为 2                                          |
| Constants.OptrCPUCost          | int32     | 操作符的 CPU 代价单位，默认值为 1                                                    |
| Constants.IOCPURate            | int32     | IO 代价与 CPU 代价的比例，默认值为 2000                                              |
| Constants.TBScanStartCost      | int32     | 全表扫描的启动代价，默认值为 0                                                       |
| Constants.IXScanStartCost      | int32     | 索引扫描的启动代价，默认值为 0                                                       |
| Options                        | bson      | 生成访问计划使用的配置项，即 SequoiaDB 的配置                                        |
| Options.optcostthreshold       | int32     | SequoiaDB 的 --optcostthreshold 选项，查询优化器忽略 IO 影响的最小的页数<br>数据页数大于阈值时，估算访问计划代价时需要计算 IO 的代价<br>默认值为 20，0 表示一直需要计算 IO 代价，-1 表示从不计算代价 |
| Options.sortbuf                | int32     | SequoiaDB 的 --sortbuf 选项<br>排序缓存大小（单位：MB），默认值为 256 ，最小值为 128 |
| Input                          | bson      | 生成访问计划使用的输入项，集合的统计信息<br>该字段在 Evaluate 选项为 true 时显示              |
| Input.Pages                    | int32     | 集合的数据页个数                                                                     |
| Input.Records                  | int64     | 集合的数据个数                                                                       |
| Input.RecordSize               | int32     | 集合的数据平均长度                                                                   |
| Input.NeedEvalIO               | boolean   | 根据 Input.Pages 和 Options.optcostthreshold 判断是否需要计算 IO 代价                |
| Input.CLEstFromStat            | boolean   | 是否使用集合的统计信息进行估算                                                       |
| Input.CLStatTime               | timestamp | 使用的集合的统计信息的生成时间                                                       |
| SearchPaths                    | array     | 每个搜索过的访问计划的估算过程                                                       |

>   **Note:**
>
>   *   Constants 字段下的值为常量不可进行设置
>   *   Options 字段下的值可以通过 SequoiaDB 的配置来设置

SearchPaths 数组的每项表示一个搜索过的访问计划，将展示以下信息：

| 字段名         | 类型      | 描述                                             |
| -------------- | --------- | ------------------------------------------------ |
| ScanType       | string    | 访问计划的扫描方式<br>"tbscan"：全表扫描<br>"ixscan"：索引扫描 |
| IndexName      | string    | 访问计划使用的索引的名称<br>全表扫描时，字段值为 ""      |
| UseExtSort     | boolean   | 访问计划是否使用非索引排序                       |
| Direction      | int32     | 访问计划使用索引时的扫描方向<br>1 表示正向扫描索引<br>-1 表示反向扫描索引 |
| Query          | bson      | 访问计划解析后的用户查询条件                     |
| IXBound        | bson      | 访问计划使用索引的查找范围<br>全表扫描时，字段值为 null    |
| NeedMatch      | boolean   | 访问计划获取记录时是否需要根据匹配符进行过滤<br>当没有查询条件或查询条件可以被索引覆盖时，NeedMatch 为 false   |
| IXEstFromStat  | boolean   | 是否使用索引的统计信息进行估算（索引扫描时显示） |
| IXStatTime     | timestamp | 使用的索引的统计信息的生成时间（索引扫描时显示） |
| Score          | double    | 评分，索引扫描时为索引的选择率（< 0.1时为候选计划），全表扫描时为匹配符的选择率 |
| IsCandidate    | boolean   | 是否为候选访问计划，不是候选计划则不进行估算<br>当访问计划满足以下条件之一时，都会被作为候选访问计划：<br>1. 索引扫描选择率 < 0.1 <br>2. 索引扫描完全匹配排序字段 <br>3. 为全表扫描 |
| IsUsed         | boolean   | 是否为最终选择的访问计划                           |
| TotalCost      | double    | 估算的代价（内部表示 单位约为 1/2000000 秒）<br>该代价不包括选择符、skip() 和 limit() 的影响 |
| ScanNode       | bson      | TBSCAN 的推演公式或 IXSCAN 推演公式<br>该字段在 Evaluate 选项为 true 时显示 |
| SortNode       | bson      | SORT 的推演公式<br>该字段在 Evaluate 选项为 true 且需要进行排序时显示 |

Evaluate 选项为 true 时将展示查询优化器的推演公式，每个需要计算的变量将以数组形式展示：

```lang-json
变量: [
  公式,
  代入数据的计算公式,
  计算结果
]
```

*   [TBSCAN的推演公式][TBSCAN]
*   [IXSCAN的推演公式][IXSCAN]
*   [SORT的推演公式][SORT]

>   **Note:**
>
>   推演公式中的代价均为内部表示，单位约为 1/2000000 秒


示例
---- 

```lang-json
{
  ...,
  "Search": {
    "Options": {
      "sortbuf": 256,
      "optcostthreshold": 20
    },
    "Constants": {
      "RandomReadIOCostUnit": 10,
      "SeqReadIOCostUnit": 1,
      "SeqWrtIOCostUnit": 2,
      "PageUnit": 4096,
      "RecExtractCPUCost": 4,
      "IXExtractCPUCost": 2,
      "OptrCPUCost": 1,
      "IOCPURate": 2000,
      "TBScanStartCost": 0,
      "IXScanStartCost": 0
    },
    "Input": {
      "Pages": 1,
      "Records": 10,
      "NeedEvalIO": false,
      "CLEstFromStat": false
    },
    "SearchPaths": [
      {
        "IsUsed": false,
        "IsCandidate": false,
        "Score": 1,
        "ScanType": "ixscan",
        "IndexName": "$id",
        "UseExtSort": false,
        "Direction": 1,
        "IXBound": {
          "_id": [
            [
              {
                "$minElement": 1
              },
              {
                "$maxElement": 1
              }
            ]
          ]
        },
        "NeedMatch": false,
        "IXEstFromStat": false
      },
      ...
    ]
  }
}
```


[^_^]:
     本文使用的所有引用及链接
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[TBSCAN]:manual/Manual/Cost_Estimation/TBSCAN.md
[IXSCAN]:manual/Manual/Cost_Estimation/IXSCAN.md
[SORT]:manual/Manual/Cost_Estimation/SORT.md
