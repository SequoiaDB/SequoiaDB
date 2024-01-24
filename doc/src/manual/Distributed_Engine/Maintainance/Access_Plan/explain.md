[^_^]:
    查看访问计划


用户可以使用 [SdbQuery.explain()][queryExplain] 命令查看查询的访问计划。当命令的 Detail 选项为 true 时，将会展示详细的访问计划。协调节点和数据节点会展示的不同的详细访问计划。

##协调节点上的详细访问计划##

协调节点上的详细访问计划包括以下内容：
- 协调节点上的访问计划信息
- 数据节点上的访问计划信息

其基本结构如下：
```lang-json
{
  { 协调节点的访问计划信息 },
  "PlanPath": {
    "Operator": "COORD-MERGE",
    { 协调节点查询上下文的访问计划信息 },
    "ChildOperators": [
      {
        { 数据节点的访问计划信息 },
        ...
      },
      ...
    ]
  }
}
```

协调节点上的访问计划包括以下信息：

|字段名      | 类型        |         描述         |
|------------|-------------|----------------------|
| NodeName   | string      | 访问计划所在的节点的名称 |
| GroupName  | string      | 访问计划所在的节点属于的复制组的名称 |
| Role       | string      | 访问计划所在的节点的角色，"coord" 表示协调节点 |
| Collection | string      | 访问计划访问的集合的名称 |
| Query      | bson        | 访问计划解析后的用户查询条件 |
| Sort       | bson        | 访问计划中的排序字段 |
| Selector   | bson        | 访问计划执行的选择符 |
| Hint       | bson        | 访问计划中指定查询使用索引的情况 |
| Skip       | int64       | 访问计划需要跳过的记录个数 |
| Return     | int64       | 访问计划最多返回的记录个数 |
| Flag       | int32       | 访问计划中指定的执行标志，默认值为 0 |
| ReturnNum  | int64       | 访问计划返回记录的个数 |
| ElapsedTime| double      | 访问计划查询耗时（单位：秒）|
| UserCPU    | double      | 访问计划用户态 CPU 使用时间（单位：秒）|
| SysCPU     | double      | 访问计划内核态 CPU 使用时间（单位：秒）|
| PlanPath   | bson        | 访问计划的具体执行操作 COORD-MERGE |

>**Note:**  
>
> COORD-MERGE 中可能包含主表的访问计划或者数据节点的访问计划

**示例**

```lang-json
{
  "NodeName": "hostname:11810",
  "GroupName": "SYSCoord",
  "Role": "coord",
  "Collection": "sample.employee",
  "Query": {
    "a": 1
  },
  "Sort": {},
  "Selector": {},
  "Hint": {
    "": ""
  },
  "Skip": 0,
  "Return": -1,
  "Flag": 0,
  "ReturnNum": 10,
  "ElapsedTime": 0.050839,
  "UserCPU": 0,
  "SysCPU": 0,
  "PlanPath": {
    "Operator": "COORD-MERGE",
    ...
  }
}
```

##数据节点上的详细访问计划##

数据节点上的详细访问计划包括以下内容：
+ 数据节点上的访问计划信息
+ 访问计划的缓存使用情况
+ 表分区中主表与子表的访问计划信息

###主表的详细访问计划###

主表的详细访问计划结构如下：

```lang-json
{
  { 主表的访问计划信息 },
  "PlanPath": {
    "Operator": "MERGE",
    { 主表查询上下文的访问计划信息 },
    "ChildOperators": [
      {
        { 子表的访问计划信息 },
        ...
      },
      ...
    ]
  }
}
```

主表访问计划包括以下信息：

| 字段名 | 类型 | 描述 |
|-------|------|-----|
| NodeName | string | 访问计划所在的节点的名称 |
| GroupName | string | 访问计划所在的节点属于的复制组的名称 |
| Role | string | 访问计划所在的节点的角色，"data" 表示协调节点 |
| Collection | string | 访问计划访问的集合的名称 |
| Query | bson | 访问计划解析后的用户查询条件 |
| Sort | bson | 访问计划中的排序字段 |
| Selector | bson | 访问计划执行的选择符 |
| Hint | bson | 访问计划中指定查询使用索引的情况 |
| Skip | int64 | 访问计划需要跳过的记录个数 |
| Return | int64 | 访问计划最多返回的记录个数 |
| Flag | int32 | 访问计划中指定的执行标志，默认值为 0 |
| ReturnNum | int64 | 访问计划返回记录的个数 |
| ElapsedTime | double | 访问计划查询耗时（单位：秒） |
| IndexRead | int64 | 访问计划扫描索引记录的个数 |
| DataRead | int64 | 访问计划扫描数据记录的个数 |
| UserCPU | double | 访问计划用户态 CPU 使用时间（单位：秒）|
| SysCPU | double | 访问计划内核态 CPU 使用时间（单位：秒）|
 PlanPath | bson | 访问计划的具体执行操作 MERGE |

> **Note:**
>
> MERGE 中包含子表的访问计划，即数据节点上的普通集合的访问计划

**示例**

```lang-json
{
  "NodeName": "hostname:11820",
  "GroupName": "group",
  "Role": "data",
  "Collection": "maincs.maincl",
  "Query": {},
  "Sort": {
    "a": 1
  },
  "Selector": {},
  "Hint": {},
  "Skip": 0,
  "Return": -1,
  "Flag": 2048,
  "ReturnNum": 50000,
  "ElapsedTime": 1.225226,
  "IndexRead": 0,
  "DataRead": 50000,
  "UserCPU": 0.5399999999999991,
  "SysCPU": 0.02000000000000002,
  "PlanPath": {
    "Operator": "MERGE",
    ...
  }
}
```

###普通集合或子表的详细访问计划###

普通集合或子表的详细访问计划结构如下：

```lang-json
{
  { 集合的访问计划信息 },
  "PlanPath": {
    { 查询上下文的访问计划信息 }
    ...
  }
}
```

该访问计划包括以下信息：

|字段名|类型|描述|
|------|----|----|
|NodeName|string|访问计划所在的节点的名称|
|GroupName|string|访问计划所在的节点属于的复制组的名称|
|Role|string|访问计划所在的节点的角色，"data" 表示数据节点|
|Collection|string|访问计划访问的集合的名称|
|Query|bson|访问计划解析后的用户查询条件|
|Sort|bson|访问计划中的排序字段|
|Selector|bson|访问计划执行的选择符|
|Hint|bson|访问计划中指定查询使用索引的情况|
|Skip|int64|访问计划需要跳过的记录个数|
|Return|int64|访问计划最多返回的记录个数|
|Flag|int32|访问计划中指定的执行标志，默认值为 0|
|ReturnNum|int64|访问计划返回记录的个数|
|ElapsedTime|double|访问计划查询耗时（单位：秒）|
|IndexRead|int64|访问计划扫描索引记录的个数|
|DataRead|int64|访问计划扫描数据记录的个数|
|UserCPU|double|访问计划用户态 CPU 使用时间（单位：秒）|
|SysCPU|double|访问计划内核态 CPU 使用时间（单位：秒）|
|CacheStatus|string|访问计划的缓存状态，取值如下：<br> "NoCache"：没有加入缓存  <br> "NewCache"：新建的缓存 <br> "HitCache"：命中的缓存|
|MainCLPlan|boolean|访问计划是否主表共享的查询计划|
|CacheLevel|string|访问计划的缓存级别，取值如下：<br> "OPT_PLAN_NOCACHE"：不进行缓存 <br> "OPT_PLAN_ORIGINAL"：缓存原查询计划 <br> "OPT_PLAN_NORMALZIED"：缓存泛化后的查询计划 <br> "OPT_PLAN_PARAMETERIZED"：缓存参数化的查询计划 <br> "OPT_PLAN_FUZZYOPTR"：缓存参数化并带操作符模糊匹配的查询计划|
|Parameters|array|参数化的访问计划使用的参数列表|
|MatchConfig|bson|访问计划中的匹配符的配置|
|MatchConfig.EnableMixCmp|boolean|访问计划的匹配符是否使用混合匹配模式|
|MatchConfig.Parameterized|boolean|访问计划的匹配符是否支持参数化|
|MatchConfig.FuzzyOptr|boolean|访问计划的匹配符是否支持模糊匹配|
|PlanPath|bson|访问计划的具体执行操作 SORT、TBSCAN 或 IXSCAN|
|Search|bson|查询计划优化器搜索过的访问计划 Search 选项为 true 时显示，详情可参考[基于代价的访问计划评估][cost_estimation]|

>**Note:**  
>
> 数据节点上的主表的访问计划可参考主表的访问计划。

**示例**

```lang-json
{
  "NodeName": "hostname:11820",
  "GroupName": "group",
  "Role": "data",
  "Collection": "sample.employee",
  "Query": {
    "a": {
      "$gt": 100
    }
  },
  "Sort": {},
  "Selector": {},
  "Hint": {},
  "Skip": 0,
  "Return": -1,
  "Flag": 2048,
  "ReturnNum": 0,
  "ElapsedTime": 0.000093,
  "IndexRead": 0,
  "DataRead": 0,
  "UserCPU": 0,
  "SysCPU": 0,
  "CacheStatus": "HitCache",
  "MainCLPlan": false,
  "CacheLevel": "OPT_PLAN_PARAMETERIZED",
  "Parameters": [
    100
  ],
  "MatchConfig": {
    "EnableMixCmp": false,
    "Parameterized": true,
    "FuzzyOptr": false
  },
  "PlanPath": {
    ...
  }
```

##访问计划中的操作##

访问计划中包含几种具体的操作，各操作的含义及包含的信息不同。

###COORD-MERGE 操作###

详细的访问计划中，COORD-MERGE 对象对应一个协调节点上的查询上下文对象，其中展示的信息如下：

|字段名|类型|描述|
|-----|---|----|
|Operator|string|操作符的名称： "COORD-MERGE"|
|Sort|string|COORD-MERGE 需要保证输出结果有序的排序字段|
|NeedReorder|boolean|COORD-MERGE 是否需要根据排序字段对多个数据组的记录进行排序合并，当查询中包含排序的时候 NeedReorder 为 true|
|DataNodeNum|int32|COORD-MERGE 涉及查询的数据节点个数|
|DataNodeList|array|COORD-MERGE 涉及查询的数据节点，按查询的执行顺序列出|
|DataNodeList.Name|string|COORD-MERGE 发送查询的数据节点名称|
|DataNodeList.EstTotalCost|double|COORD-MERGE 发送的查询在该数据节点上查询的估算时间（单位：秒）|
|DataNodeList.QueryTimeSpent|double|在数据节点上查询的执行时间（单位：秒），Run 选项为 true 时显示|
|DataNodeList.WaitTimeSpent|double|COORD-MERGE 发送的查询在数据节点上查询的等待时间（单位：秒），Run 选项为 true 时显示|
|Selector|bson|COORD-MERGE 执行的选择符|
|Skip|int64|指定 COORD-MERGE 需要跳过的记录个数|
|Return|int64|指定 COORD-MERGE 最多返回的记录个数|
|Estimate|bson|估算的 COORD-MERGE 代价信息，Estimate 选项为 true 时显示|
|Estimate.StartCost|double|估算的 COORD-MERGE 的启动时间（单位：秒）|
|Estimate.RunCost|double|估算的 COORD-MERGE的运行时间（单位：秒）|
|Estimate.TotalCost|double|估算的 COORD-MERGE 的结束时间（单位：秒）|
|Estimate.Output|bson|估算的 COORD-MERGE 输出结果的统计信息，Filter 选项包含 "Output" 时显示|
|Estimate.Output.Records|int64|估算的 COORD-MERGE 输出的记录个数|
|Estimate.Output.RecordSize|int32|估算的 COORD-MERGE 输出的记录平均字节数|
|Estimate.Output.Sorted|boolean|COORD-MERGE 输出结果是否有序|
|Run|bson|实际执行 COORD-MERGE 的代价信息，Run 选项为 true 时显示|
|Run.ContextID|int64|COORD-MERGE 执行的上下文 ID|
|Run.StartTimestamp|string|COORD-MERGE 执行启动的时间戳|
|Run.QueryTimeSpent|double|COORD-MERGE 执行耗时（单位：秒）|
|Run.GetMores|int64|请求 COORD-MERGE 返回结果集的次数|
|Run.ReturnNum|int64|COORD-MERGE 返回记录个数|
|Run.WaitTimeSpent|double|COORD-MERGE 等待数据返回的时间（单位：秒，以秒为单位的粗略统计信息）|
|ChildOperators|array|COORD-MERGE 的子操作（每个数据组返回的查询的访问计划结果），详情可参考[数据节点的访问计划][explain]|


**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "COORD-MERGE",
    "Sort": {},
    "NeedReorder": false,
    "DataNodeNum": 2,
    "DataNodeList": [
      {
        "Name": "hostname:11820",
        "EstTotalCost": 0.4750005,
        "QueryTimeSpent": 0.045813,
        "WaitTimeSpent": 0.000124
      },
      {
        "Name": "hostname:11830",
        "EstTotalCost": 0.4750005,
        "QueryTimeSpent": 0.045841,
        "WaitTimeSpent": 0.000108
      }
    ],
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 0,
      "RunCost": 0.4750015,
      "TotalCost": 0.4750015,
      "Output": {
        "Records": 2,
        "RecordSize": 43,
        "Sorted": false
      }
    },
    "Run": {
      "ContextID": 9,
      "StartTimestamp": "2017-12-09-13.51.14.749863",
      "QueryTimeSpent": 0.046311,
      "GetMores": 3,
      "ReturnNum": 10,
      "WaitTimeSpent": 0
    },
    "ChildOperators": [
      {
        ...
      },
      {
        ...
      }
    ]
  }
}
```

###MERGE 操作###

详细的访问计划中，MERGE 对象对应一个数据节点上的主表查询上下文对象，其中展示的信息如下：

|字段名|类型|描述|
|------|----|----|
|Operator|string|操作符的名称："MERGE"|
|Sort|string|MERGE 需要保证输出结果有序的排序字段|
|NeedReorder|boolean|MERGE 是否需要根据排序字段对多个子表的记录进行排序合并，当查询中包含排序，排序字段不包含主表的分区键时为 true|
|SubCollectionNum|int32|MERGE 涉及查询的子表个数|
|SubCollectionList|array|MERGE 涉及查询的子表，按查询的执行顺序列出|
|SubCollectionList.Name|string|MERGE 发送查询的子表名称|
|SubCollectionList.EstTotalCost|double|MERGE 发送的查询在子表上查询的估算时间（单位：秒）|
|SubCollectionList.QueryTimeSpent|double|MERGE 发送的查询在子表上查询的执行时间（单位：秒），Run 选项为 true 时显示|
|SubCollectionList.WaitTimeSpent|double|MERGE 发送的查询在数据节点上查询的等待时间（单位：秒），Run 选项为 true 时显示）|
|Selector|bson|MERGE 执行的选择符|
|Skip|int64|指定 MERGE 需要跳过的记录个数|
|Return|int64|指定 MERGE 最多返回的记录个数|
|Estimate|bson|估算的 MERGE 代价信息，Estimate 选项为 true 时显示|
|Estimate.StartCost|int64|估算的 MERGE 的启动时间（单位：秒）|
|Estimate.RunCost|int64|估算的 MERGE 的运行时间（单位：秒）|
|Estimate.TotalCost|int64|估算的 MERGE 的结束时间（单位：秒）|
|Estimate.Output|bson|估算的 MERGE 输出结果的统计信息，Filter 选项包含"Output"时显示|
|Estimate.Output.Records|int64|估算的 MERGE 输出的记录个数|
|Estimate.Output.RecordSize|int32|估算的 MERGE 输出的记录平均字节数|
|Estimate.Output.Sorted|boolean|MERGE 输出结果是否有序|
|Run|bson|实际执行 MERGE 的代价信息，Run 选项为 true 时显示|
|Run.ContextID|int64|MERGE 执行的上下文 ID|
|Run.StartTimestamp|string|MERGE 执行启动的时间戳|
|Run.QueryTimeSpent|double|MERGE 执行耗时（单位：秒）|
|Run.GetMores|int64|请求 MERGE 返回结果集的次数|
|Run.ReturnNum|int64|MERGE 返回记录个数|
|SubCollections|array|MERGE 的子操作（每个子表返回的查询的访问计划结果），详细可参考[数据节点的访问计划][explain]|


**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "MERGE",
    "Sort": {
      "h": 1
    },
    "NeedReorder": true,
    "SubCollectionNum": 2,
    "SubCollectionList": [
      {
        "Name": "subcs.subcl1",
        "EstTotalCost": 0.8277414999999999,
        "QueryTimeSpent": 1.080046,
        "WaitTimeSpant": 0.000234
      },
      {
        "Name": "subcs.subcl2",
        "EstTotalCost": 0.8277414999999999,
        "QueryTimeSpent": 0.946832,
        "WaitTimeSpant": 0.000182
      }
    ],
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 1.630483,
      "RunCost": 0.09999999999999999,
      "TotalCost": 1.730483,
      "Output": {
        "Records": 50000,
        "RecordSize": 43,
        "Sorted": true
      }
    },
    "Run": {
      "ContextID": 63121,
      "StartTimestamp": "2017-12-11-16.18.00.789234",
      "QueryTimeSpent": 1.203218,
      "GetMores": 3,
      "ReturnNum": 50000
    },
    "SubCollections": [
      {
        ...
      },
      {
        ...
      }
    ]
  }
}
```

###SORT 操作###

详细的访问计划中，SORT 对象对应一个数据节点上的排序上下文对象，其中展示的信息如下：

|字段名|类型|描述|
|------|----|----|
|Operator|string|操作符的名称："SORT"|
|Sort|bson|SORT 执行的排序字段|
|Selector|bson|SORT 执行的选择符|
|Skip|int64|指定 SORT 需要跳过的记录个数|
|Return|int64|指定 SORT 最多返回的记录个数|
|Estimate|bson|估算的 SORT 代价信息，Estimate 选项为 true 时显示|
|Estimate.StartCost|double|估算的 SORT 的启动时间（单位：秒）|
|Estimate.RunCost|double|估算的 SORT 的运行时间（单位：秒）|
|Estimate.TotalCost|double|估算的 SORT 的结束时间（单位：秒）|
|Estimate.SortType|string|SORT 估算的排序类型，取值如下：<br>"InMemory"：内存排序 <br> "External"：外存排序|
|Estimate.Output|bson|估算的 SORT 输出的统计信息，Filter 选项包含 "Output" 时显示|
|Estimate.Output.Records|int64|估算的 SORT 输出的记录个数|
|Estimate.Output.RecordSize|int32|估算的 SORT 输出的记录平均字节数|
|Estimate.Output.Sorted|boolean|SORT 输出是否有序，对 SORT 为 true|
|Run|bson|实际查询的 SORT 代价信息，Run 选项为 true 时显示|
|Run.ContextID|int64|SORT 执行的上下文 ID|
|Run.StartTimestamp|string|SORT 启动的时间|
|Run.QueryTimeSpent|double|SORT 耗时（单位：秒）|
|Run.GetMores|int64|请求 SORT 返回结果集的次数|
|Run.ReturnNum|int64|SORT 返回记录个数|
|Run.SortType|string|SORT 执行的排序类型，取值如下：<br> "InMemory"：内存排序 <br> "External"：外存排序|
|ChildOperators|array|SORT 的子操作（TBSCAN 或 IXSCAN）|

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "SORT",
    "Sort": {
      "c": 1
    },
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 0.475,
      "RunCost": 5e-7,
      "TotalCost": 0.4750005,
      "SortType": "InMemory",
      "Output": {
        "Records": 1,
        "RecordSize": 43,
        "Sorted": true
      }
    },
    "Run": {
      "ContextID": 8,
      "StartTimestamp": "2017-11-29-14.02.38.108504",
      "QueryTimeSpent": 0.050564,
      "GetMores": 1,
      "ReturnNum": 5,
      "SortType": "InMemory"
    },
    "ChildOperators": [
      {
        ...
      }
    ]
  }
}
```

###TBSCAN 操作###
详细的访问计划中，TBSCAN 对应一个使用全表扫描的上下文对象，展示的信息如下：

|字段名|类型|描述|
|------|----|----|
|Operator|string|操作符的名称："TBSCAN"|
|Collection|string|TBSCAN 访问的集合名字|
|Query|bson|TBSCAN 执行的匹配符|
|Selector|bson|TBSCAN 执行的选择符|
|Skip|int64|指定 TBSCAN 需要跳过的记录个数|
|Return|int64|指定 TBSCAN 最多返回的记录个数|
|Estimate|bson|估算的 TBSCAN 代价信息，Estimate 选项为 true 时显示|
|Estimate.StartCost|double|估算的 TBSCAN 的启动时间（单位：秒）|
|Estimate.RunCost|double|估算的 TBSCAN 的运行时间（单位：秒）|
|Estimate.TotalCost|double|估算的 TBSCAN 的结束时间（单位：秒）|
|Estimate.CLEstFromStat|boolean|TBSCAN 是否使用集合的统计信息进行估算|
|Estimate.CLStatTime|string|TBSCAN 使用的集合的统计信息的生成时间|
|Estimate.Input|bson|估算的 TBSCAN 输入的统计信息，Filter 选项包含 "Input" 时显示|
|Estimate.Input.Pages|int64|估算的 TBSCAN 输入的数据页数|
|Estimate.Input.Records|int64|估算的 TBSCAN 输入的记录个数|
|Estimate.Input.RecordSize|int32|估算的 TBSCAN 输入的记录平均字节数|
|Estimate.Filter|bson|估算的 TBSCAN 进行过滤的信息，Filter 选项包含 "Filter" 时显示|
|Estimate.Filter.MthSelectivity|double|估算的 TBSCAN 使用匹配符进行过滤的选择率|
|Estimate.Output|bson|估算的 TBSCAN 输出的统计信息，Filter 选项包含 "Output" 时显示|
|Estimate.Output.Records|int64|估算的 TBSCAN 输出的记录个数|
|Estimate.Output.RecordSize|int32|估算的 TBSCAN 输出的记录平均字节数|
|Estimate.Output.Sorted|boolean|TBSCAN 输出是否有序，对 TBSCAN 为 false|
|Run|bson|实际执行 TBSCAN 的代价信息，Run 选项为 true 时显示|
|Run.ContextID|int64|TBSCAN 执行的上下文标识|
|Run.StartTimestamp|string|TBSCAN 执行启动的时间戳|
|Run.QueryTimeSpent|double|TBSCAN 执行耗时（单位：秒）|
|Run.GetMores|int64|请求 TBSCAN 返回结果集的次数|
|Run.ReturnNum|int64|TBSCAN 返回记录个数|
|Run.ReadRecords|int64|TBSCAN 扫描记录个数|

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "TBSCAN",
    "Collection": "sample.employee",
    "Query": {
      "$and": []
    },
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 0,
      "RunCost": 0.45,
      "TotalCost": 0.45,
      "CLEstFromStat": false,
      "Input": {
        "Pages": 25,
        "Records": 25000,
        "RecordSize": 43
      },
      "Filter": {
        "MthSelectivity": 1
      },
      "Output": {
        "Records": 25000,
        "RecordSize": 43,
        "Sorted": false
      }
    },
    "Run": {
      "ContextID": 63123,
      "StartTimestamp": "2017-12-11-16.18.00.789831",
      "QueryTimeSpent": 0.040438,
      "GetMores": 25,
      "ReturnNum": 25000,
      "ReadRecords": 25000
    }
  }
}
```

###IXSCAN 操作###
详细的访问计划中，IXSCAN 对应一个使用索引扫描的上下文对象，展示的信息如下：

|字段名|类型|描述|
|------|----|----|
|Operator|string|操作符的名称： "IXSCAN"|
|Collection|string|IXSCAN 访问集合的名字|
|Index|string|IXSCAN 访问索引的名字|
|IXBound|bson|IXSCAN 访问索引的查找范围|
|Query|bson|IXSCAN 执行的匹配符|
|NeedMatch|boolean|IXSCAN 是否需要在数据上执行匹配符进行过滤|
| IndexCover     | boolean    | 访问计划匹配条件字段、选择字段、排序字段是否被索引覆盖。被索引覆盖可以直接使用索引键值替代集合记录，提升访问性能 |
|Selector|bson|IXSCAN 执行的选择符|
|Skip|int64|指定 IXSCAN 需要跳过的记录个数|
|Return|int64|指定 IXSCAN 最多返回的记录个数|
|Estimate|bson|估算的 IXSCAN 代价信息，Estimate 选项为 true 时显示|
|Estimate.StartCost|double|估算的 IXSCAN 的启动时间（单位：秒）|
|Estimate.RunCost|double|估算的 IXSCAN 的运行时间（单位：秒）|
|Estimate.TotalCost|double|估算的 IXSCAN 的结束时间（单位：秒）|
|Estimate.CLEstFromStat|boolean|IXSCAN 是否使用集合的统计信息进行估算|
|Estimate.CLStatTime|timestamp|IXSCAN 使用的集合的统计信息的生成时间|
|Estimate.IXEstFromStat|boolean|IXSCAN 是否使用索引的统计信息进行估算|
|Estimate.IXStatTime|timestamp|IXSCAN 使用的索引的统计信息的生成时间|
|Estimate.Input|bson|估算的 IXSCAN 输入的统计信息，Filter 选项包含 "Input" 时显示|
|Estimate.Input.Pages|int64|估算的 IXSCAN 输入的数据页数|
|Estimate.Input.Records|int64|估算的 IXSCAN 输入的记录个数|
|Estimate.Input.RecordSize|int32|估算的 IXSCAN 输入的记录平均字节数|
|Estimate.Input.IndexPages|int32|估算的 IXSCAN 输入的索引页数|
|Estimate.Filter|bson|估算的 IXSCAN 进行过滤的信息，Filter 选项包含 "Filter" 时显示|
|Estimate.Filter.MthSelectivity|double|估算的 IXSCAN 使用匹配符进行过滤的选择率|
|Estimate.Filter.IXScanSelectivity|double|估算的 IXSCAN 使用索引时需要扫描索引的比例|
|Estimate.Filter.IXPredSelectivity|double|估算的 IXSCAN 使用索引进行过滤的选择率|
|Estimate.Output|bson|估算的 IXSCAN 输出的统计信息，Filter 选项包含 "Output" 时显示|
|Estimate.Output.Records|int64|估算的 IXSCAN 输出的记录个数|
|Estimate.Output.RecordSize|int32|估算的 IXSCAN 输出的记录平均字节数|
|Estimate.Output.Sorted|boolean|IXSCAN 输出是否有序，如果索引包含 Sort 的所有字段并且匹配顺序，该项为 true，否则为 false|
|Run|bson|实际查询的 IXSCAN 代价信息，Run 选项为 true 时显示|
|Run.ContextID|int64|IXSCAN 执行的上下文 ID|
|Run.StartTimestamp|string|IXSCAN 启动的时间|
|Run.QueryTimeSpent|double|IXSCAN 耗时（单位：秒）|
|Run.GetMores|int64|请求 IXSCAN 返回结果集的次数|
|Run.ReturnNum|int64|IXSCAN 返回记录个数|
|Run.ReadRecords|int64|IXSCAN 扫描数据记录个数|
|Run.IndexReadRecords|int64|IXSCAN 扫描索引项个数|

**示例**

```lang-json
{
  ...,
  "PlanPath": {
    "Operator": "IXSCAN",
    "Collection": "sample.employee",
    "Index": "index",
    "IXBound": {
      "a": [
        [
          1,
          1
        ]
      ]
    },
    "Query": {
      "$and": [
        {
          "a": {
            "$et": 1
          }
        }
      ]
    },
    "NeedMatch": false,
    "IndexCover": true,
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Estimate": {
      "StartCost": 5e-7,
      "RunCost": 0.3200035,
      "TotalCost": 0.320004,
      "CLEstFromStat": false,
      "IXEstFromStat": false,
      "Input": {
        "Pages": 25,
        "Records": 25000,
        "RecordSize": 43,
        "IndexPages": 15
      },
      "Filter": {
        "MthSelectivity": 0.00004,
        "IXScanSelectivity": 0.00004,
        "IXPredSelectivity": 0.00004,
      },
      "Output": {
        "Records": 1,
        "RecordSize": 43,
        "Sorted": false
      }
    },
    "Run": {
      "ContextID": 36136,
      "StartTimestamp": "2017-12-11-16.11.34.518111",
      "QueryTimeSpent": 0.935198,
      "GetMores": 1,
      "ReturnNum": 5,
      "ReadRecords": 5,
      "IndexReadRecords": 6
    }
  }
}
```

[^_^]:
  本文用到的所有链接
[queryExplain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[cost_estimation]:manual/Distributed_Engine/Maintainance/Access_Plan/cost_estimation.md
[explain]:manual/Distributed_Engine/Maintainance/Access_Plan/explain.md#数据节点上的详细访问计划