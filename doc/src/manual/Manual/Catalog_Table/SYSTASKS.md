
SYSCAT.SYSTASKS 集合中包含了该集群中正在运行的后台任务信息，每个任务保存为一个文档。

每个文档包含以下字段：

| 字段名            | 类型   | 描述                                              |
|-------------------|--------|---------------------------------------------------|
| TaskID            | number | 任务 ID |
| TaskType          | number | 任务类型，取值如下：<br> 0：数据切分<br> 1：清除序列缓存<br> 2：创建索引<br> 3：删除索引<br> 4：复制索引 |
| TaskTypeDesc      | string | 任务类型描述，与字段 TaskType 对应，取值如下：<br> "Split"<br> "Alter sequence"<br> "Create index"<br> "Drop index"<br> "Copy index" |
| Status            | number | 任务状态，取值如下：<br> 0：准备<br> 1：运行<br> 2：暂停<br> 3：取消<br> 4：变更元数据<br> 5：清理数据<br> 6：回滚<br> 9：完成<br> |
| StatusDesc        | string | 任务状态描述，与字段 Status 对应，取值如下：<br> "Ready"<br> "Running"<br> "Pause"<br> "Canceled"<br> "Change meta"<br> "Clean up"<br> "Roll back"<br> "Finish"<br> |
| Name              | string | 集合名 |
| UniqueID          | number | 集合的唯一 ID ( 仅切分任务显示 ) |
| ResultCode        | number | 错误码，当任务失败时，错误码被设置 |
| ResultCodeDesc    | string | 错误码描述 |
| ResultInfo        | object | 错误详细信息 |
| CreateTimestamp   | string | 创建任务的时间 |
| BeginTimestamp    | string | 任务开始运行的时间，即状态转换为 Running 的时间 |
| EndTimestamp      | string | 任务结束的时间 |
| ShardingKey       | object | 分区键 ( 仅切分任务显示 ) |
| ShardingType      | string | 分区类型 ( 仅切分任务显示 )，取值如下：<br> "hash"<br> "range" |
| Source            | string | 源分区所在数据组名 ( 仅切分任务显示 ) |
| SourceID          | number | 源分区所在数据组 ID ( 仅切分任务显示 ) |
| TargetName        | string | 目标分区所在数据组名 ( 仅切分任务显示 ) |
| TargetID          | number | 目标分区所在数据组 ID ( 仅切分任务显示 ) |
| SplitValue        | object | 范围切分条件 ( 仅切分任务显示 ) |
| SplitEndValue     | object | 范围切分结束条件 ( 仅切分任务显示 ) |
| SplitPercent      | number | 切分百分比 ( 仅切分任务显示 ) |
| IndexDef          | object | 索引定义 ( 仅创建索引任务显示 ) |
| IndexName         | string | 索引名 ( 仅创建/删除索引任务显示 ) |
| SortBufferSize    | number | 排序缓存的大小，单位为 MB ( 仅创建索引任务显示 ) |
| CopyTo            | array  | 复制索引的目的子表列表 ( 仅复制索引任务显示 ) |
| IndexNames        | array  | 复制索引的索引列表 ( 仅复制索引任务显示 ) |
| FailedGroups      | number | 执行失败的数据组个数 ( 仅索引任务显示 ) |
| SucceededGroups   | number | 执行成功的数据组个数 ( 仅索引任务显示 ) |
| TotalGroups       | number | 总共的数据组个数 ( 仅索引任务显示 ) |
| Groups            | array  | 执行任务的数据组信息 ( 仅索引任务显示 ) |
| Groups.GroupName      | string | 数据组名 |
| Groups.Status         | number | 该数据组的任务状态 |
| Groups.StatusDesc     | string | 该数据组的任务状态描述 |
| Groups.ResultCode     | number | 该数据组的错误码 |
| Groups.ResultCodeDesc | string | 该数据组的错误码描述 |
| Groups.ResultInfo     | object | 该数据组的错误详细信息 |
| Groups.RetryCount     | number | 该数据组的重试次数 |
| Groups.OpInfo         | string | 该数据组的操作信息，描述内部正在执行的操作 |
| Groups.Progress       | number | 该数据组的任务进度，单位：% |
| Groups.Speed          | number | 该数据组的执行速度，单位：条/秒 |
| Groups.TimeLeft       | number | 该数据组的预计剩余时间，单位：秒 |
| Groups.TimeSpent      | number | 该数据组的已花费时间，单位：秒 |
| Groups.TotalRecords   | number | 该数据组的集合总记录数 |
| Groups.ProcessedRecords | number | 该数据组的已处理的记录数 |
| IsMainTask        | boolean | 是否为主任务 ( 仅主任务显示 ) |
| FailedSubTasks    | number | 执行失败的子任务个数 ( 仅主任务显示 ) |
| SucceededSubTasks | number | 执行成功的子任务个数 ( 仅主任务显示 ) |
| TotalSubTasks     | number | 总共的子任务个数 ( 仅主任务显示 ) |
| SubTasks          | array  | 子任务信息 ( 仅主任务显示 ) |
| SubTasks.TaskID       | number | 子任务的任务 ID |
| SubTasks.TaskType     | number | 子任务的任务类型 |
| SubTasks.Status       | number | 子任务的状态 |
| SubTasks.StatusDesc   | string | 子任务的状态描述 |
| SubTasks.ResultCode   | number | 子任务的错误码 |
| Progress          | number | 任务进度，单位：% |
| Speed             | number | 执行速度，单位：条/秒 |
| TimeLeft          | number | 预计剩余时间，单位：秒 |
| TimeSpent         | number | 已花费时间，单位：秒 |

**示例**

- 切分任务信息如下：

 ```lang-json
 {
   "BeginTimestamp": "2021-05-10-10.37.36.859725",
   "EndTimestamp": "2021-05-10-10.37.42.361991",
   "Name": "sample.employee",
   "ResultCode": 0,
   "ResultCodeDesc": "Succeed",
   "ShardingKey": {
     "a": 1
   },
   "ShardingType": "hash",
   "Source": "group1",
   "SourceID": 1000,
   "SplitEndValue": {},
   "SplitPercent": 20,
   "SplitValue": {
     "": 4015
   },
   "Status": 9,
   "StatusDesc": "Finish",
   "Target": "group2",
   "TargetID": 1001,
   "TaskID": 1698,
   "TaskType": 0,
   "UniqueID": 3221225472001,
   "_id": {
     "$oid": "60989c6d2b72db1816b04eaa"
   }
 }
```



