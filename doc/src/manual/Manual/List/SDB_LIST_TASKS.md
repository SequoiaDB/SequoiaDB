后台任务列表可以列出正在运行的后台任务信息。

##标识##

SDB_LIST_TASKS

##字段信息##

字段信息可参考 [SYSTASKS 集合][SYSTASKS]。

##示例##

查看后台任务列表

```lang-javascript
> db.list(SDB_LIST_TASKS)
```

输出结果如下：

```lang-javascript
{
  "_id": {
    "$oid": "61dd364e7c3048cfb366e8bc"
  },
  "TaskID": 1,
  "TaskType": 0,
  "Status": 0,
  "ResultCode": 0,
  "Name": "business.orders_2019",
  "UniqueID": 4294967297,
  "SourceID": 1000,
  "Source": "group1",
  "TargetID": 1001,
  "Target": "group2",
  "SplitValue": {
    "": 2048
  },
  "SplitEndValue": {},
  "ShardingKey": {
    "id": 1
  },
  "ShardingType": "hash",
  "SplitPercent": 0,
  "EndTimestamp": ""
}
```

[^_^]:
    本文使用的所有引用及链接
[SYSTASKS]:manual/Manual/Catalog_Table/SYSTASKS.md
