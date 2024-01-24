
##NAME##

explain - Get the access plan for the query.

##SYNOPSIS##

***query.explain( [options] )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the access plan for the query.

##PARAMETERS##

| Name    | Type     | Description                      | Required or not |
| ------- | -------- | -------------------------------- | --------------- |
| options | JSON     | Access plan execution parameters | not             |

The detail description of 'options' parameter is as follow:

| Attributes | Type | Default | Description |
| ---------- | ---- | ------- | ----------- |
| Run            | bool | false | Whether to execute the access plan.<br>Execute an access plan to get data and time informaitom if Run is true. <br>Otherwise it only gets information about the access and the access plan isn't executed |
| Detail         | bool | false | Whether to display more detailed information about access plan, like coord nodes, data nodes and context information that related to the access plans. <br>Display more detailed information about access plan if Detail is true. <br>Otherwise it doesn't display them. <br>However, if Detail is true, it displays one level more detailed information about access plan. <br>You can use parameter Expand to control whether to display all more detailed information about access plan. |
| Estimate       | bool | ( same as Detail ) | Whether to display the estimate part of the detailed access plan. <br>Display the estimate part of the detailed access plan if Estimate is true. <br>Otherwise it doesn't display them. <br>If the Estimate option is explicitly set, Detail is automatically set to true. |
| Expand         | bool | false | Whether to display all level more detailed information about the access plan. <br>Display all level more detailed information about the access plan if Expand is true. <br>Otherwise it display one level more detailed information about the access plan.<br>If Expand is explicitly set, Detail is automatically set to true. |
| Flatten        | bool | false | Indicates whether to expand the output of the access plan for each node and each child tables as a record. Expand the output if Flatten is true. <br>Otherwise, it will not. <br>If Flatten is explicitly set, Detail and Expand are automatically set to true. |
| Filter         | string / string array | "All" | Fileter the details of the estimation results. The filter option allows you to select "None", "Output", "Input", "Filter", "All", or a combination of them. <br>None: No detail of the estimated results are output.<br>Input: Output details of the intput of the estimation result. <br>Filter: Output filtering details of the estimation results. <br>Output: Output details of the output of the estimation result. <br>All: output full details of the estimation results.  <br>If Filter is explicitly set, Detail and Estimate are automatically set to true. |
| CMDLocation    | JSON | --- | The results of the access plan are filtered according to the data set, using the command location parameter item. The CMDLocation option only supports the "GroupID" and "GroupName" options.<br>If CMDLocation is explicitly set, Detail is automatically set to true. |
| SubCollections | string / string array | --- | The results of the access plan are filtered according to the sub-table. <br>The SubCollections option only takes effect when an access plan with primary child tables is used. <br>The SubCollections option can select a sub-table name, or an array of sub-table names, to indicate that only the access plan for the specified sub-table is displayed. <br>The SubCollections option is [] or null for no filtering. <br>If SubCollections is explicitly set, Detail is automatically set to true. |
| Search         | bool | false | Whether to view the access plan that the query optimizer has searched for and to view the results of the query optimizer selection. <br>Show the selection process of the query optimizer if Search is true. <br>Otherwise, it won't show them.<br>If Search  is explicitly set, Detail and Expand are automatically set to true. |
| Evaluate       | bool | false | Whether to view the calculation process of the access plan that the query optimizer has searched for. <br>Shows the calculation process of the query optimizer if Evaluate is true. <br>Otherwise it won't show them.<br>If Evaluate is explicitly set, Detail, Search and Expand are automatically set to true. |
| Abbrev         | bool | false | Whether to output strings in abbreviation mode. |

##Normal access plan##

When the Detail option is false, a normal access plan will be displayed. Access plan information for common collections are as follow:

| FieldName   | Type   | Description |
| ----------- | ------ | ----------- |
| NodeName    | string | the name of the node where the accss plan is located |
| GroupName   | string | the name of the replication group where the access plan is located |
| Role        | string | the role of the node where the access plan is located. "data" represents a data node. "coord" represents a coordination node |
| Name        | string | the name of the collection where the access plan accessed. |
| ScanType    | string | the scan type of the access plan. "tbscan" represents full tables scan. "ixscan" represents index scan |
| IndexName   | string | the name of the index that the access plan uses. The index name is "" if the scan type is full tables scan |
| UseExtSort  | bool   | non-index |
| Query       | BSON   | the user query condition after analysising access plan |
| IXBound     | BSON   | the index's search range that access plan uses and it is null if the scan is a table scan |
| NeedMatch   | bool   | whether to filter according to the match when access plan gets the record. If NeedMatch is false, it means there is no query conditions or query conditions can be covered by the index |
| IndexCover  | bool   | whether the matching condition field, selection field and sorting field of access plan  are covered by the index.if it is covered by index, we can use index value instead of record to improve access performance |
| ReturnNum   | long   | the number of records that access plan returns |
| ElapsedTime | double | the query time of access plan( unit: s ) |
| IndexRead   | long   | the number of index records that access plan scanns |
| DataRead    | long   | the number of data records that access plan scanns |
| UserCPU     | double | user mode CPU usage time( unit: s ) |
| SysCPU      | bouble | kernel state CPU usage time( unit: s ) |

Access plan information for the primary table in the table partition:

| FieldName      | Type   | Description |
| -------------- | ------ | ----------- |
| NodeName       | string | the name of the node where access plan is located |
| GroupName      | string | the replication group name of the node where access plan is located |
| Role           | string | the node's role where access plan is located |
| Name           | string | the collection name where access plan is located |
| SubCollections | array  | access plan for each subtable in the table partition   |

Access plan information for subtables in a table partition:

| FieldName      | Type   | Description |
| -------------- | ------ | ----------- |
| Name        | string | the collection name where access plan is located |
| ScanType    | string | access plan's scanning method. "tbscan" means full table scan and "ixscan" means index scan |
| IndexName   | string | the name of index that access plan uses. If it is full table scan, the IndexName is "" |
| UseExtSort  | bool   | non-index |
| Query       | BSON   | the user query condition after analysising access plan |
| IXBound     | BSON   | the index's search range that access plan uses and it is null if the scan is a table scan |
| NeedMatch   | bool   | whether to filter according to the match when access plan gets the record. If NeedMatch is false, it means there is no query conditions or query conditions can be covered by the index |
| IndexCover  | bool   | whether the matching condition field, selection field and sorting field of access plan  are covered by the index.if it is covered by index, we can use index value instead of record to improve access performance |
| ReturnNum   | long   | the number of records that access plan returns |
| ElapsedTime | double | the query time of access plan( unit: s ) |
| IndexRead   | long   | the number of index records that access plan scanns |
| DataRead    | long   | the number of data records that access plan scanns |
| UserCPU     | double | user mode CPU usage time( unit: s ) |
| SysCPU      | bouble | kernel state CPU usage time( unit: s ) |

>**Note:**

>1. If the collection is distributed across multiple replication groups, the access plan is returned as a set of records. 

>2. If the match of the query cannot hit any of the partitions of the table partiton, the query will not be sebt to the data node for execution, and access plan at this time will return a  virtual access plan with the coordination node.

##Detailed access plan##

When the Detail option is false, a detailed access plan will be displayed. The detailed access plans presented on the coordination node and the data node are slightly different. For more detial, please reference to [detailed access plan](database_management/access_plans/detailed_access_plan/overview.md). 

When the Search is true, it will view the access plan that the query optimizer has searched for and to view the results of the query optimizer selection. For more detial, please reference to [access plan's search process](database_management/access_plans/search_paths/overview.md).

##RETURN VALUE##

On success, return access plan's cursor.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Normal access plan.

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

* Table partition access plan.

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
      "IndexCover": true,
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

* The vittual access plan on the coordination node. Matches cannot hit any partition.

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

* View the general access plan for the query and execute query using the Run option.

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
  "IndexCover": falses,
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

* Use the Detail option to view the detailed access plan for the query.

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

* Use the Detail option to view the detailed access plan for the query and execute query using the Run option.

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

* Use the Detail option to view the detailed access plan for the query and use Search option to view the search optimizer's search process.

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
              "IndexCover": false,
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
              "IndexCover":false,
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
              "IndexCover":false,
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

* Use the Detail option to view the detailed access plan for the query and use CMDLocation option to view the query's access plan on group1.

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

* Use the Detail option to view the detailed access plan for table partitions and use Expand option to expand all detailed information.

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


