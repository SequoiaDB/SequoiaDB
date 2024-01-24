[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190320
    评审意见
    
    王涛：
    许建辉：
    市场部：


访问计划缓存快照可以列出数据库中缓存的访问计划信息。

标识
----

SDB_SNAP_ACCESSPLANS

字段信息
----

| 字段名               | 类型      | 描述                                           |
| -------------------- | --------- | ---------------------------------------------- |
| NodeName             | string    | 该访问计划所在的节点名，格式为<主机名>:<服务名>|
| GroupName            | string    | 该访问计划所在的数据组名                       |
| AccessPlanID         | int64    | 该访问计划的 ID 标识                           |
| Collection           | string    | 该访问计划所在的集合名                         |
| CollectionSpace      | string    | 该访问计划所在的集合空间名                     |
| ScanType             | string    | 该访问计划的扫描类型，取值如下：<br/>"tbscan"：表扫描<br/>"ixscan"：索引扫描 |
| IndexName            | string    | 该访问计划使用的索引，表扫描时该项为空字符串   |
| UseExtSort           | boolean   | 该访问计划是否需要对扫描结果进行排序           |
| CacheLevel           | string    | 该访问计划的缓存级别，取值如下：<br/>"OPT_PLAN_NOCACHE"：不进行缓存<br/>"OPT_PLAN_ORIGINAL"：缓存原访问计划<br/>"OPT_PLAN_NORMALZIED"：缓存泛化后的访问计划<br/>"OPT_PLAN_PARAMETERIZED"：缓存参数化的访问计划<br/>"OPT_PLAN_FUZZYOPTR"：缓存参数化并带操作符模糊匹配的访问计划 |
| Query                | bson      | 该访问计划的匹配符                             |
| Sort                 | bson      | 该访问计划的排序字段                           |
| Hint                 | bson      | 该访问计划指定使用索引的提示                   |
| SortedIndexRequired  | boolean   | 该访问计划是否需要使用根据 Sort 排序的索引     |
| EstFromStat          | boolean    | 该访问计划是否使用统计信息生成                 |
| HashCode             | int32     | 该访问计划的 hash 值                           |
| Score                | double    | 该访问计划的评分                               |
| RefCount             | int32     | 该访问计划正在被使用的查询个数                 |
| ParamPlanValid       | boolean   | 该访问计划是否参数化有效的访问计划             |
| MainCLPlanValid      | boolean   | 该访问计划是否主表有效的访问计划               |
| ValidParams          | bson array| 该访问计划已经生效的参数列表，仅在 ParamPlanValid 为 false 时显示  |
| ValidParams.Parameters | array   | 该访问计划生效的参数                            |
| ValidParams.Score      | double | 该访问计划生效的参数的评分                      |
| ValidSubCLs          | bson array| 该访问计划已经生效的参数列表，仅在 MainCLPlanValid 为 false 时显示 |
| ValidSubCLs.Collection | string | 该访问计划为主表访问计划时生效的子表名          |
| ValidSubCLs.Parameters | array   | 该访问计划生效的参数                            |
| ValidSubCLs.Score      | double | 该访问计划生效的参数的评分                      |
| AccessCount          | int64     | 该访问计划使用次数累计                         |
| TotalQueryTime       | double    | 该访问计划的累计执行时间，单位为秒             |
| AvgQueryTime         | double    | 该访问计划的平均执行时间，单位为秒             |
| MaxTimeSpentQuery    | bson      | 该访问计划执行时间最慢的一次查询信息           |
| MinTimeSpentQuery    | bson      | 该访问计划执行时间最快的一次查询信息           |


**MaxTimeSpentQuery 和 MinTimeSpentQuery 对象的字段**

| 字段名               | 类型      | 描述                                           |
| -------------------- | --------- | ---------------------------------------------- |
| ContextID            | int64     | 查询所在的上下文 ID                            |
| QueryType            | string    | 查询的类型，取值如下：<br/>SELECT：数据查询<br/>UPDATE：数据更新<br/>DELETE：数据删除 |
| QueryTimeSpent       | double    | 查询执行的时间，单位为秒                     |
| ExecuteTimeSpent     | double    | 额外操作（UPDATE 或者 DELETE）执行的时间，单位为秒  |
| Parameters           | bson array| 查询使用的参数                                 |
| StartTimestamp       | string    | 查询启动的时间戳                               |
| Selector             | bson      | 查询使用的选择符                               |
| Skip                 | int64     | 查询需要跳过的记录个数                         |
| Return               | int64     | 查询需要最多返回的记录个数                     |
| Flag                 | int32     | 查询使用的标志位                               |
| DataRead             | int64     | 查询读取的记录个数                             |
| IndexRead            | int64     | 查询通过索引读取的记录个数                     |
| GetMores             | int32     | 查询返回的结果集次数                           |
| ReturnNum            | int64     | 查询返回的记录个数                             |
| HitEnd               | boolean   | 查询是否返回所有需要的结果                     |

 > **Note:**  
 > - 最慢和最快的查询 MaxTimeSpentQuery 和 MinTimeSpentQuery 只计算查询的 QueryTimeSpent，不计算查询的 ExecuteTimeSpent。  
 > - SdbQuery.explain() 的 Run 模式不计算在 QueryTimeSpent、MaxTimeSpentQuery 和 MinTimeSpentQuery 中。

示例
----

查看访问计划缓存快照

```lang-javascript
> db.snapshot( SDB_SNAP_ACCESSPLANS )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:11820",
  "GroupName": "group1",
  "AccessPlanID": 14,
  "Collection": "sample.employee",
  "CollectionSpace": "sample",
  "ScanType": "tbscan",
  "IndexName": "",
  "UseExtSort": false,
  "CacheLevel": "OPT_PLAN_PARAMETERIZED",
  "Query": {
    "$and": [
      {
        "a": {
          "$et": {
            "$param": 0,
            "$ctype": 10
          }
        }
      }
    ]
  },
  "Sort": {},
  "Hint": {},
  "SortedIndexRequired": false,
  "EstFromStat": false,
  "HashCode": -1411136567,
  "Score": 0,
  "RefCount": 0,
  "ParamPlanValid": true,
  "AccessCount": 5,
  "TotalQueryTime": 0.254869,
  "AvgQueryTime": 0.0509738,
  "MaxTimeSpentQuery": {
    "ContextID": 155,
    "QueryType": "SELECT",
    "Parameters": [
      4
    ],
    "StartTimestamp": "2017-12-09-14.03.57.909232",
    "QueryTimeSpent": 0.060943,
    "ExecuteTimeSpent": 0,
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Flag": 2048,
    "DataRead": 25000,
    "IndexRead": 0,
    "GetMores": 1,
    "ReturnNum": 0,
    "HitEnd": true
  },
  "MinTimeSpentQuery": {
    "ContextID": 157,
    "QueryType": "SELECT",
    "Parameters": [
      2
    ],
    "StartTimestamp": "2017-12-09-14.03.59.073811",
    "QueryTimeSpent": 0.040419,
    "ExecuteTimeSpent": 0,
    "Selector": {},
    "Skip": 0,
    "Return": -1,
    "Flag": 2048,
    "DataRead": 25000,
    "IndexRead": 0,
    "GetMores": 1,
    "ReturnNum": 0,
    "HitEnd": true
  }
}
```
