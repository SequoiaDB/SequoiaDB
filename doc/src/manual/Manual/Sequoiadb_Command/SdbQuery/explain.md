##名称##

explain - 获取查询的访问计划

##语法##

**query.explain( \[options\] )**

##类别##

SdbQuery

##描述##

获取查询的访问计划。

##参数##

| 参数名  | 参数类型  | 描述 | 是否必填 |
| ------- | --------- | ---- | -------- |
| options | BSON 对象 | 访问计划执行参数 | 否 |

options 参数详细说明如下：

| 参数名         | 参数类型         | 描述   | 默认值 |
| -------------- | ---------------- | ------ | ------ |
| Run            | 布尔型           | 表示是否执行访问计划。<br>Run 选项为 true 表示执行访问计划，获取数据和时间信息。<br>Run 选项为 false 表示只获取访问计划的信息，并不执行。 | false |
| Detail         | 布尔型           | 表示是否展示更详细的访问计划，展示涉及访问计划的协调节点、数据节点及相关的上下文信息。<br>Detail 选项为 true 表示展示详细的访问计划。<br>Detail 选项为 true 时默认展示一层详细的访问计划，需要使用 Expand 选项展示所有详细的访问计划。<br>Detail 选项为 false 表示不展示详细的访问计划。 | false |
| Estimate       | 布尔型           | 表示是否展示详细的访问计划中的估算部分。<br>Estimate 选项为 true 表示展示详细的访问计划中的估算部分。<br>Estimate 选项为 false 表示不展示详细的访问计划中的估算部分。<br>如果 Estimate 选项显式设置，Detail 自动设置为 true。 | Detail 选项的值 |
| Expand         | 布尔型           | 表示是否展示详细的访问计划中的多层信息。<br>Expand 选项为 true 表示展示详细的访问计划中的多层信息。<br>Expand 选项为 false 表示仅展示一层详细的访问计划，该层的子层将不展开。<br>如果 Expand 选项显式设置，Detail 自动设置为 true。 | false |
| Flatten        | 布尔型           | 表示是否展开每个节点和每个子表的访问计划的输出结果作为一条记录。<br>Flatten 选项为 true 表示展开输出结果。<br>Flatten 选项为 false 表示不展开输出结果，并组合成数组挂在上一级节点或者主表上展示。<br>如果 Flatten 选项显式设置，Detail 选项和 Expand 选项自动设置为 true。 | false |
| Filter         | 字符串 或者 数组 | 表示对估算结果（Estimate 选项）的细节进行过滤。<br>Filter 选项为可以选择 "None"、"Output"、"Input"、"Filter"、"All"，或者其组合数组。<br>Filter 选项为 [] 或者 null 表示为 "None"。<br>1. "None" 表示不输出估算结果的任何细节。<br>2. "Input" 表示输出估算结果的输入细节。<br>3. "Filter" 表示输出估算结果的过滤细节。<br>4. "Output" 表示输出估算结果的输出细节。<br>5. "All" 表示输出估算结果的全部细节。<br>如果 Filter 选项显式设置，Detail 选项和 Estimate 选项自动设置为 true。 | "All" |
| CMDLocation    | BSON 对象        | 表示对访问计划的结果按照数据组进行过滤，使用命令位置参数项。<br>CMDLocation 选项仅支持 "GroupID" 和 "GroupName" 选项。<br>详细见 **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)**。<br>如果 CMDLocation 选项显式设置，Detail 选项自动设置为 true。 | 空 |
| SubCollections | 字符串 或者 数组 | 表示对访问计划的结果按照子表进行过滤。<br>SubCollections 选项只在带有主子表的访问计划时生效。<br>SubCollections 选项可以选择某个子表名，或者子表名的数组，表示只显示指定子表的访问计划。<br>SubCollections 选项为 [] 或者 null 表示为不进行过滤。<br>如果 SubCollections 选项显式设置，Detail 选项自动设置为 true。  | 空 |
| Search         | 布尔型           | 表示是否查看查询优化器搜索过的访问计划，并查看查询优化器选择的结果。<br>Search 选项为 true 表示展示查询优化器的选择过程。<br>Search 选项为 false 表示不展示查询优化器的选择过程。<br>如果 Search 选项显式设置，Detail 选项和 Expand 选项自动设置为 true。 | false |
| Evaluate       | 布尔型           | 表示是否查看查询优化器搜索过的访问计划的计算过程。<br>Evaluate 选项为 true 表示展示查询优化器的计算过程。<br>Evaluate 选项为 false 表示不展示查询优化器的计算过程。<br>如果 Evaluate 选项显式设置，Detail 选项、Search 选项和 Expand 选项自动设置成 true。 | false |
| Abbrev         | 布尔型           | 表示是否对过长的字符串进行简略输出。| false |

##普通的访问计划##

Detail 选项为 false 时，将会展示普通的访问计划。

普通集合的访问计划信息：

| 字段名         | 类型      | 描述                                             |
| -------------- | --------- | ------------------------------------------------ |
| NodeName       | 字符串    | 访问计划所在的节点的名称                         |
| GroupName      | 字符串    | 访问计划所在的节点属于的复制组的名称             |
| Role           | 字符串    | 访问计划所在的节点的角色<br>1. "data" 表示数据节点<br>2. "coord" 表示协调节点 |
| Name           | 字符串    | 访问计划访问的集合的名称                         |
| ScanType       | 字符串    | 访问计划的扫描方式<br>1. "tbscan" 表示全表扫描<br>2. "ixscan" 表示索引扫描 |
| IndexName      | 字符串    | 访问计划使用的索引的名称<br>全表扫描时，字段值为 ""      |
| UseExtSort     | 布尔型    | 访问计划是否使用非索引排序                       |
| Query          | BSON 对象 | 访问计划解析后的用户查询条件                     |
| IXBound        | BSON 对象 | 访问计划使用索引的查找范围<br>全表扫描时，字段值为 null  |
| NeedMatch      | 布尔型    | 访问计划获取记录时是否需要根据匹配符进行过滤<br>NeedMatch 为 false 的情况有：<br>1. 没有查询条件<br>2. 查询条件可以被索引覆盖 |
| IndexCover     | 布尔型    | 访问计划匹配条件字段、选择字段、排序字段是否被索引覆盖。<br>被索引覆盖可以直接使用索引键值替代集合记录，提升访问性能 |
| ReturnNum      | 长整型    | 访问计划返回记录的个数                           |
| ElapsedTime    | 浮点数    | 访问计划查询耗时（单位：秒）                     |
| IndexRead      | 长整型    | 访问计划扫描索引记录的个数                       |
| DataRead       | 长整型    | 访问计划扫描数据记录的个数                       |
| UserCPU        | 浮点数    | 访问计划用户态 CPU 使用时间（单位：秒）          |
| SysCPU         | 浮点数    | 访问计划内核态 CPU 使用时间（单位：秒）          |

表分区中主表的访问计划信息：

| 字段名         | 类型      | 描述                                             |
| -------------- | --------- | ------------------------------------------------ |
| NodeName       | 字符串    | 访问计划所在的节点的名称                         |
| GroupName      | 字符串    | 访问计划所在的节点属于的复制组的名称             |
| Role           | 字符串    | 访问计划所在的节点的角色<br>1. "data" 表示数据节点<br>2. "coord" 表示协调节点 |
| Name           | 字符串    | 访问计划访问的集合的名称                         |
| SubCollections | 数组      | 表分区中各子表的访问计划                         |

表分区中子表的访问计划信息：

| 字段名         | 类型      | 描述                                             |
| -------------- | --------- | ------------------------------------------------ |
| Name           | 字符串    | 访问计划访问的集合的名称                         |
| ScanType       | 字符串    | 访问计划的扫描方式<br>1. "tbscan" 表示全表扫描<br>2. "ixscan" 表示索引扫描 |
| IndexName      | 字符串    | 访问计划使用的索引的名称<br>全表扫描时，字段值为 ""      |
| UseExtSort     | 布尔型    | 访问计划是否使用非索引排序                       |
| Query          | BSON 对象 | 访问计划解析后的用户查询条件                     |
| IXBound        | BSON 对象 | 访问计划使用索引的查找范围<br>全表扫描时，字段值为 null    |
| NeedMatch      | 布尔型    | 访问计划获取记录时是否需要根据匹配符进行过滤<br>NeedMatch 为 false 的情况有：<br>1. 没有查询条件<br>2. 查询条件可以被索引覆盖 |
| IndexCover     | 布尔型    | 访问计划匹配条件字段、选择字段、排序字段是否被索引覆盖。<br>被索引覆盖可以直接使用索引键值替代集合记录，提升访问性能 |
| ReturnNum      | 长整型    | 访问计划返回记录的个数                           |
| ElapsedTime    | 浮点数    | 访问计划查询耗时（单位：秒）                     |
| IndexRead      | 长整型    | 访问计划扫描索引记录的个数                       |
| DataRead       | 长整型    | 访问计划扫描数据记录的个数                       |
| UserCPU        | 浮点数    | 访问计划用户态 CPU 使用时间（单位：秒）          |
| SysCPU         | 浮点数    | 访问计划内核态 CPU 使用时间（单位：秒）          |

>   **Note:**
>
>   1.  如果集合经过 split 分布在多个复制组，访问计划会按照一组一记录的方式返回。
>   2.  如果查询的匹配符不能命中表分区的任何一个分区时，查询将不会下发到数据节点上执行，此时的访问计划将返回一个带有协调节点的虚拟访问计划。

### 详细的访问计划

Detail 选项为 true 时，将会展示[详细的访问计划][explain]。在协调节点和数据节点上展示的详细访问计划略有不同。


### 访问计划的搜索过程

Search 选项为 true 时，将会展示[访问计划的搜索过程][cost_estimation]，并查看查询优化器选择的结果。

##返回值##

返回访问计划的游标，类型为 object 。

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。
关于错误处理可以参考[常见错误处理指南][faq]。

常见错误可参考[错误码][error_code]。

##版本##

v2.0 及以上版本。

##示例##

* 普通表的访问计划

    ```lang-json
    {
      "Name": "sample.employee",
      "ScanType": "ixscan",
      "IndexName": "$shard",
      "UseExtSort": false,
      "Query": {
        "$and": [
          {
            "a": {
              "$gt": 1
            }
          }
        ]
      },
      "IXBound": {
        "a": [
          [
            1,
            {
              "$maxElement": 1
            }
          ]
        ]
      },
      "NeedMatch": false,
      "IndexCover": true,
      "NodeName": "hostname:11830",
      "GroupName": "group",
      "Role": "data",
      "ReturnNum": 0,
      "ElapsedTime": 0.000107,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0,
      "SysCPU": 0
    }
    ```

* 表分区的访问计划

    ```lang-json
    {
      "NodeName": "hostname:11830",
      "GroupName": "group",
      "Role": "data",
      "Name": "maincs.maincl",
      "SubCollections": [
        {
          "Name": "subcs.subcl1",
          "ScanType": "tbscan",
          "IndexName": "",
          "UseExtSort": false,
          "Query": {
            "$and": []
          },
          "IXBound": null,
          "NeedMatch": false,
          "IndexCover": false,
          "ReturnNum": 0,
          "ElapsedTime": 0.000088,
          "IndexRead": 0,
          "DataRead": 0,
          "UserCPU": 0,
          "SysCPU": 0
        },
        {
          "Name": "subcs.subcl2",
          "ScanType": "tbscan",
          "IndexName": "",
          "UseExtSort": false,
          "Query": {
            "$and": []
          },
          "IXBound": null,
          "NeedMatch": false,
          "IndexCover": true,
          "ReturnNum": 0,
          "ElapsedTime": 0.000089,
          "IndexRead": 0,
          "DataRead": 0,
          "UserCPU": 0,
          "SysCPU": 0
        }
      ]
    }
    ```

* 协调节点上的虚拟访问计划，即匹配符不能命中任何分区

    ```lang-json
    {
      "NodeName": "hostname:11810",
      "GroupName": "SYSCoord",
      "Role": "coord",
      "Collection": "maincs.maincl",
      "Query": {
        "a": 10000000
      },
    }
    ```

* 查看查询的普通访问计划，并使用 Run 选项执行查询

    ```lang-javascript
    > db.sample.employee.find( { a : { $gt : 100 } } ).explain( { Run : true } )
    {
      "NodeName": "hostname:11810",
      "GroupName": "group1",
      "Role": "data",
      "Name": "sample.employee",
      "ScanType": "tbscan",
      "IndexName": "",
      "UseExtSort": false,
      "Query": {
        "$and": [
          {
            "a": {
              "$gt": 100
            }
          }
        ]
      },
      "IXBound": null,
      "NeedMatch": true,
      "IndexCover": false,
      "ReturnNum": 49892,
      "ElapsedTime": 0.323423,
      "IndexRead": 0,
      "DataRead": 49945,
      "UserCPU": 0.1399999999999999,
      "SysCPU": 0
    }
    {
      "NodeName": "hostname:11820",
      "GroupName": "group2",
      "Role": "data",
      "Name": "sample.employee",
      "ScanType": "tbscan",
      "IndexName": "",
      "UseExtSort": false,
      "Query": {
        "$and": [
          {
            "a": {
              "$gt": 100
            }
          }
        ]
      },
      "IXBound": null,
      "NeedMatch": true,
      "IndexCover": false,
      "ReturnNum": 50007,
      "ElapsedTime": 0.41887,
      "IndexRead": 0,
      "DataRead": 50055,
      "UserCPU": 0.1400000000000006,
      "SysCPU": 0.009999999999999787
    }
    ```

* 使用 Detail 选项查看查询的详细访问计划

    ```lang-javascript
    > db.sample.employee.find( { a : { $gt : 100 } } ).explain( { Detail : true } )
    {
      "NodeName": "hostname:11800",
      "GroupName": "SYSCoord",
      "Role": "coord",
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
      "Flag": 0,
      "ReturnNum": 0,
      "ElapsedTime": 0.00123,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0,
      "SysCPU": 0,
      "PlanPath": {
        "Operator": "COORD-MERGE",
        "Sort": {},
        "NeedReorder": false,
        "DataNodeNum": 2,
        "DataNodeList": [
          {
            "Name": "hostname:11810",
            "EstTotalCost": 1.484
          },
          {
            "Name": "hostname:11820",
            "EstTotalCost": 0.7418349999999999
          }
        ],
        "Selector": {},
        "Skip": 0,
        "Return": -1,
        "Estimate": {
          "StartCost": 0,
          "RunCost": 1.5214865,
          "TotalCost": 1.5214865,
          "Output": {
            "Records": 74973,
            "RecordSize": 29,
            "Sorted": false
          }
        },
        "ChildOperators": [
          {
            "NodeName": "hostname:11810",
            "GroupName": "group1",
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
            "ElapsedTime": 0.000078,
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
            }
          },
          {
            "NodeName": "hostname:11820",
            "GroupName": "group2",
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
            "ElapsedTime": 0.000081,
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
            }
          }
        ]
      }
    }
    ```

* 使用 Detail 选项查看查询的详细访问计划，并使用 Run 选项执行查询

    ```lang-javascript
    > db.sample.employee.find( { a : { $gt : 100 } } ).explain( { Detail : true, Run : true } )
    {
      "NodeName": "hostname:11800",
      "GroupName": "SYSCoord",
      "Role": "coord",
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
      "Flag": 0,
      "ReturnNum": 99899,
      "ElapsedTime": 0.82863,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0.01999999999999999,
      "SysCPU": 0.009999999999999995,
      "PlanPath": {
        "Operator": "COORD-MERGE",
        "Sort": {},
        "NeedReorder": false,
        "DataNodeNum": 2,
        "DataNodeList": [
          {
            "Name": "hostname:11820",
            "EstTotalCost": 0.7418349999999999,
            "QueryTimeSpent": 0.733299,
            "WaitTimeSpent": 0.013556
          },
          {
            "Name": "hostname:11810",
            "EstTotalCost": 1.484,
            "QueryTimeSpent": 0.82677,
            "WaitTimeSpent": 0.084652
          }
        ],
        "Selector": {},
        "Skip": 0,
        "Return": -1,
        "Estimate": {
          "StartCost": 0,
          "RunCost": 1.5214865,
          "TotalCost": 1.5214865,
          "Output": {
            "Records": 74973,
            "RecordSize": 29,
            "Sorted": false
          }
        },
        "Run": {
          "ContextID": 29314,
          "StartTimestamp": "2017-12-14-15.24.51.254623",
          "QueryTimeSpent": 0.821182,
          "GetMores": 112,
          "ReturnNum": 99899,
          "WaitTimeSpent": 0.075
        },
        "ChildOperators": [
          {
            "NodeName": "hostname:11820",
            "GroupName": "group2",
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
            "ReturnNum": 49892,
            "ElapsedTime": 0.733493,
            "IndexRead": 0,
            "DataRead": 49945,
            "UserCPU": 0.14,
            "SysCPU": 0.01000000000000001,
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
            }
          },
          {
            "NodeName": "hostname:11810",
            "GroupName": "group1",
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
            "ReturnNum": 50007,
            "ElapsedTime": 0.82666,
            "IndexRead": 0,
            "DataRead": 50055,
            "UserCPU": 0.1499999999999986,
            "SysCPU": 0.01000000000000023,
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
            }
          }
        ]
      }
    }
    ```

* 使用 Detail 选项查看查询的详细访问计划，并且使用 Search 选项查看查询优化器的搜索过程

    ```lang-javascript
    > db.sample.employee.find( { a : { $gt : 100 } } ).explain( { Detail : true, Search : true } )
    {
      "NodeName": "hostname:11800",
      "GroupName": "SYSCoord",
      "Role": "coord",
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
      "Flag": 0,
      "ReturnNum": 0,
      "ElapsedTime": 0.037223,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0,
      "SysCPU": 0,
      "PlanPath": {
        "Operator": "COORD-MERGE",
        "Sort": {},
        "NeedReorder": false,
        "DataNodeNum": 2,
        "DataNodeList": [
          {
            "Name": "hostname:11820",
            "EstTotalCost": 0.7418349999999999
          },
          {
            "Name": "hostname:11810",
            "EstTotalCost": 1.334165
          }
        ],
        "Selector": {},
        "Skip": 0,
        "Return": -1,
        "Estimate": {
          "StartCost": 0,
          "RunCost": 1.3591655,
          "TotalCost": 1.3591655,
          "Output": {
            "Records": 50001,
            "RecordSize": 29,
            "Sorted": false
          }
        },
        "ChildOperators": [
          {
            "NodeName": "hostname:11820",
            "GroupName": "group2",
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
            "ElapsedTime": 0.000048,
            "IndexRead": 0,
            "DataRead": 0,
            "UserCPU": 0,
            "SysCPU": 0,
            "CacheStatus": "NoCache",
            "MatchConfig": {
              "EnableMixCmp": false,
              "Parameterized": false,
              "FuzzyOptr": false
            },
            "PlanPath": {
              "Operator": "TBSCAN",
              "Collection": "sample.employee",
              "Query": {
                "$and": [
                  {
                    "a": {
                      "$gt": 100
                    }
                  }
                ]
              },
              "Selector": {},
              "Skip": 0,
              "Return": -1,
              "Estimate": {
                "StartCost": 0,
                "RunCost": 0.7418349999999999,
                "TotalCost": 0.7418349999999999,
                "CLEstFromStat": false,
                "Input": {
                  "Pages": 37,
                  "Records": 49945,
                  "RecordSize": 29
                },
                "Filter": {
                  "MthSelectivity": 0.4999994999999995
                },
                "Output": {
                  "Records": 24973,
                  "RecordSize": 29,
                  "Sorted": false
                }
              }
            },
            "Search": {
              "Options": {
                "sortbuf": 256,
                "optcostthreshold": 20
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
                  "NeedMatch": true,
                  "IndexCover": false,
                  "IXEstFromStat": false
                },
                {
                  "IsUsed": false,
                  "IsCandidate": false,
                  "Score": 0.4999994999999995,
                  "ScanType": "ixscan",
                  "IndexName": "$shard",
                  "UseExtSort": false,
                  "Direction": 1,
                  "IXBound": {
                    "a": [
                      [
                        100,
                        {
                          "$decimal": "MAX"
                        }
                      ]
                    ]
                  },
                  "NeedMatch": false,
                  "IXEstFromStat": false
                },
                {
                  "IsUsed": true,
                  "IsCandidate": true,
                  "Score": 0.4999994999999995,
                  "TotalCost": 1483670,
                  "ScanType": "tbscan",
                  "IndexName": "",
                  "UseExtSort": false
                }
              ]
            }
          },
          {
            "NodeName": "hostname:11810",
            "GroupName": "group1",
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
            "ElapsedTime": 0.000064,
            "IndexRead": 0,
            "DataRead": 0,
            "UserCPU": 0,
            "SysCPU": 0,
            "CacheStatus": "NoCache",
            "MatchConfig": {
              "EnableMixCmp": false,
              "Parameterized": false,
              "FuzzyOptr": false
            },
            "PlanPath": {
              "Operator": "TBSCAN",
              "Collection": "sample.employee",
              "Query": {
                "$and": [
                  {
                    "a": {
                      "$gt": 100
                    }
                  }
                ]
              },
              "Selector": {},
              "Skip": 0,
              "Return": -1,
              "Estimate": {
                "StartCost": 0,
                "RunCost": 1.334165,
                "TotalCost": 1.334165,
                "CLEstFromStat": false,
                "Input": {
                  "Pages": 74,
                  "Records": 50055,
                  "RecordSize": 29
                },
                "Filter": {
                  "MthSelectivity": 0.4999994999999995
                },
                "Output": {
                  "Records": 25028,
                  "RecordSize": 29,
                  "Sorted": false
                }
              }
            },
            "Search": {
              "Options": {
                "sortbuf": 256,
                "optcostthreshold": 20
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
                  "NeedMatch": true,
                  "IndexCover": false,
                  "IXEstFromStat": false
                },
                {
                  "IsUsed": false,
                  "IsCandidate": false,
                  "Score": 0.4999994999999995,
                  "ScanType": "ixscan",
                  "IndexName": "$shard",
                  "UseExtSort": false,
                  "Direction": 1,
                  "IXBound": {
                    "a": [
                      [
                        100,
                        {
                          "$decimal": "MAX"
                        }
                      ]
                    ]
                  },
                  "NeedMatch": false,
                  "IndexCover": false,
                  "IXEstFromStat": false
                },
                {
                  "IsUsed": true,
                  "IsCandidate": true,
                  "Score": 0.4999994999999995,
                  "TotalCost": 2668330,
                  "ScanType": "tbscan",
                  "IndexName": "",
                  "UseExtSort": false
                }
              ]
            }
          }
        ]
      }
    }
    ```

* 使用 Detail 选项查看查询的详细访问计划，并且使用 CMDLocation 选项查看查询在 group1 上的访问计划

    ```lang-javascript
    > db.sample.employee.find( { a : { $gt : 100 } } ).explain( { Detail : true, CMDLocation : { GroupName : 'group1' } } )
    {
      "NodeName": "hostname:11800",
      "GroupName": "SYSCoord",
      "Role": "coord",
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
      "Flag": 0,
      "ReturnNum": 0,
      "ElapsedTime": 0.011374,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0,
      "SysCPU": 0,
      "PlanPath": {
        "Operator": "COORD-MERGE",
        "Sort": {},
        "NeedReorder": false,
        "DataNodeNum": 2,
        "DataNodeList": [
          {
            "Name": "hostname:11810",
            "EstTotalCost": 1.484
          },
          {
            "Name": "hostname:11820",
            "EstTotalCost": 0.7418349999999999
          }
        ],
        "Selector": {},
        "Skip": 0,
        "Return": -1,
        "Estimate": {
          "StartCost": 0,
          "RunCost": 1.5214865,
          "TotalCost": 1.5214865,
          "Output": {
            "Records": 74973,
            "RecordSize": 29,
            "Sorted": false
          }
        },
        "ChildOperators": [
          {
            "NodeName": "hostname:11810",
            "GroupName": "group1",
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
            "ElapsedTime": 0.000088,
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
            }
          }
        ]
      }
    }
    ```

* 使用 Detail 选项查看表分区的详细访问计划，并使用 Expand 选项展开所有细节

    ```lang-javascript
    > db.maincs.maincl.find( { a : { $gt : 100 } } ).explain( { Detail : true, Expand : true } )
    {
      "NodeName": "hostname:11800",
      "GroupName": "SYSCoord",
      "Role": "coord",
      "Collection": "maincs.maincl",
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
      "Flag": 0,
      "ReturnNum": 0,
      "ElapsedTime": 0.002748,
      "IndexRead": 0,
      "DataRead": 0,
      "UserCPU": 0,
      "SysCPU": 0,
      "PlanPath": {
        "Operator": "COORD-MERGE",
        "Sort": {},
        "NeedReorder": false,
        "DataNodeNum": 2,
        "DataNodeList": [
          {
            "Name": "hostname:11810",
            "EstTotalCost": 0.9624999999999999
          },
          {
            "Name": "hostname:11820",
            "EstTotalCost": 0.9624999999999999
          }
        ],
        "Selector": {},
        "Skip": 0,
        "Return": -1,
        "Estimate": {
          "StartCost": 0,
          "RunCost": 0.9874999999999999,
          "TotalCost": 0.9874999999999999,
          "Output": {
            "Records": 50000,
            "RecordSize": 43,
            "Sorted": false
          }
        },
        "ChildOperators": [
          {
            "NodeName": "hostname:11810",
            "GroupName": "group1",
            "Role": "data",
            "Collection": "maincs.maincl",
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
            "ElapsedTime": 0.00062,
            "IndexRead": 0,
            "DataRead": 0,
            "UserCPU": 0,
            "SysCPU": 0,
            "PlanPath": {
              "Operator": "MERGE",
              "Sort": {},
              "NeedReorder": false,
              "SubCollectionNum": 2,
              "SubCollectionList": [
                {
                  "Name": "subcs.subcl2",
                  "EstTotalCost": 0.475
                },
                {
                  "Name": "subcs.subcl1",
                  "EstTotalCost": 0.475
                }
              ],
              "Selector": {},
              "Skip": 0,
              "Return": -1,
              "Estimate": {
                "StartCost": 0,
                "RunCost": 0.9624999999999999,
                "TotalCost": 0.9624999999999999,
                "Output": {
                  "Records": 25000,
                  "RecordSize": 43,
                  "Sorted": false
                }
              },
              "SubCollections": [
                {
                  "Collection": "subcs.subcl2",
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
                  "ElapsedTime": 0.000042,
                  "IndexRead": 0,
                  "DataRead": 0,
                  "UserCPU": 0,
                  "SysCPU": 0,
                  "CacheStatus": "HitCache",
                  "MainCLPlan": true,
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
                    "Operator": "TBSCAN",
                    "Collection": "subcs.subcl2",
                    "Query": {
                      "$and": [
                        {
                          "a": {
                            "$gt": 100
                          }
                        }
                      ]
                    },
                    "Selector": {},
                    "Skip": 0,
                    "Return": -1,
                    "Estimate": {
                      "StartCost": 0,
                      "RunCost": 0.475,
                      "TotalCost": 0.475,
                      "CLEstFromStat": false,
                      "Input": {
                        "Pages": 25,
                        "Records": 25000,
                        "RecordSize": 43
                      },
                      "Filter": {
                        "MthSelectivity": 0.4999994999999995
                      },
                      "Output": {
                        "Records": 12500,
                        "RecordSize": 43,
                        "Sorted": false
                      }
                    }
                  }
                },
                {
                  "Collection": "subcs.subcl1",
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
                  "ElapsedTime": 0.000049,
                  "IndexRead": 0,
                  "DataRead": 0,
                  "UserCPU": 0,
                  "SysCPU": 0,
                  "CacheStatus": "HitCache",
                  "MainCLPlan": true,
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
                    "Operator": "TBSCAN",
                    "Collection": "subcs.subcl1",
                    "Query": {
                      "$and": [
                        {
                          "a": {
                            "$gt": 100
                          }
                        }
                      ]
                    },
                    "Selector": {},
                    "Skip": 0,
                    "Return": -1,
                    "Estimate": {
                      "StartCost": 0,
                      "RunCost": 0.475,
                      "TotalCost": 0.475,
                      "CLEstFromStat": false,
                      "Input": {
                        "Pages": 25,
                        "Records": 25000,
                        "RecordSize": 43
                      },
                      "Filter": {
                        "MthSelectivity": 0.4999994999999995
                      },
                      "Output": {
                        "Records": 12500,
                        "RecordSize": 43,
                        "Sorted": false
                      }
                    }
                  }
                }
              ]
            }
          },
          {
            "NodeName": "hostname:11820",
            "GroupName": "group2",
            "Role": "data",
            "Collection": "maincs.maincl",
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
            "ElapsedTime": 0.00067,
            "IndexRead": 0,
            "DataRead": 0,
            "UserCPU": 0,
            "SysCPU": 0,
            "PlanPath": {
              "Operator": "MERGE",
              "Sort": {},
              "NeedReorder": false,
              "SubCollectionNum": 2,
              "SubCollectionList": [
                {
                  "Name": "subcs.subcl2",
                  "EstTotalCost": 0.475
                },
                {
                  "Name": "subcs.subcl1",
                  "EstTotalCost": 0.475
                }
              ],
              "Selector": {},
              "Skip": 0,
              "Return": -1,
              "Estimate": {
                "StartCost": 0,
                "RunCost": 0.9624999999999999,
                "TotalCost": 0.9624999999999999,
                "Output": {
                  "Records": 25000,
                  "RecordSize": 43,
                  "Sorted": false
                }
              },
              "SubCollections": [
                {
                  "Collection": "subcs.subcl2",
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
                  "ElapsedTime": 0.000034,
                  "IndexRead": 0,
                  "DataRead": 0,
                  "UserCPU": 0,
                  "SysCPU": 0,
                  "CacheStatus": "HitCache",
                  "MainCLPlan": true,
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
                    "Operator": "TBSCAN",
                    "Collection": "subcs.subcl2",
                    "Query": {
                      "$and": [
                        {
                          "a": {
                            "$gt": 100
                          }
                        }
                      ]
                    },
                    "Selector": {},
                    "Skip": 0,
                    "Return": -1,
                    "Estimate": {
                      "StartCost": 0,
                      "RunCost": 0.475,
                      "TotalCost": 0.475,
                      "CLEstFromStat": false,
                      "Input": {
                        "Pages": 25,
                        "Records": 25000,
                        "RecordSize": 43
                      },
                      "Filter": {
                        "MthSelectivity": 0.4999994999999995
                      },
                      "Output": {
                        "Records": 12500,
                        "RecordSize": 43,
                        "Sorted": false
                      }
                    }
                  }
                },
                {
                  "Collection": "subcs.subcl1",
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
                  "ElapsedTime": 0.000048,
                  "IndexRead": 0,
                  "DataRead": 0,
                  "UserCPU": 0,
                  "SysCPU": 0,
                  "CacheStatus": "HitCache",
                  "MainCLPlan": true,
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
                    "Operator": "TBSCAN",
                    "Collection": "subcs.subcl1",
                    "Query": {
                      "$and": [
                        {
                          "a": {
                            "$gt": 100
                          }
                        }
                      ]
                    },
                    "Selector": {},
                    "Skip": 0,
                    "Return": -1,
                    "Estimate": {
                      "StartCost": 0,
                      "RunCost": 0.475,
                      "TotalCost": 0.475,
                      "CLEstFromStat": false,
                      "Input": {
                        "Pages": 25,
                        "Records": 25000,
                        "RecordSize": 43
                      },
                      "Filter": {
                        "MthSelectivity": 0.4999994999999995
                      },
                      "Output": {
                        "Records": 12500,
                        "RecordSize": 43,
                        "Sorted": false
                      }
                    }
                  }
                }
              ]
            }
          }
        ]
      }
    }
    ```


[^_^]:
     本文使用的所有引用及链接
[explain]:manual/Distributed_Engine/Maintainance/Access_Plan/explain.md
[cost_estimation]:manual/Distributed_Engine/Maintainance/Access_Plan/cost_estimation.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
