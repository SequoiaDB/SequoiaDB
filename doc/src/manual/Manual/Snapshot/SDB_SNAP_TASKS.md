
任务快照可以列出当前数据库中所有数据节点的任务信息。

##标识##

SDB_SNAP_TASKS

##字段信息##

| 字段名         | 数据类型 | 说明 |
| -------------- | -------- | ---- |
| NodeName       | string   | 任务所属节点名，格式为<主机名>:<端口号> |
| TaskID         | uint64   | 任务 ID，与 [listTasks()][listTasks] 的 TaskID 对应 |
| Status         | int32    | 任务状态，取值如下：<br> 0：准备<br> 1：运行<br> 2：暂停<br> 3：取消<br> 4：变更元数据<br> 5：清理数据<br> 6：回滚<br> 9：完成<br> |
| StatusDesc     | string   | 任务状态描述，与参数 Status 对应，取值如下：<br> "Ready"<br> "Running"<br> "Pause"<br> "Canceled"<br> "Change meta"<br> "Clean up"<br> "Roll back"<br> "Finish"<br> |
| TaskType       | int32    | 任务类型，取值如下：<br> 2：创建索引<br> 3：删除索引 |
| TaskTypeDesc   | string   | 任务类型描述，与参数 TaskType 对应，取值如下：<br> "Create index"<br> "Drop index" |
| Name           | string   | 集合名 |
| IndexName      | string   | 索引名 |
| IndexDef       | json     | 索引定义 |
| SortBufferSize | int32    | 排序缓存的大小，单位为 MB ( 仅创建索引任务显示 ) |
| ResultCode     | int32    | 错误码，当任务失败时，将显示对应的错误码 |
| ResultCodeDesc | string   | 错误码描述 |
| ResultInfo     | json     | 错误详细信息 |
| OpInfo         | string   | 描述该任务正在执行的操作 |
| RetryCount     | uint32   | 重试次数 |
| Progress       | uint32   | 任务进度，单位为% |
| Speed          | uint64   | 任务执行速度，单位为 条/秒 |
| TimeLeft       | double   | 预计剩余时间，单位为秒 |
| TimeSpent      | double   | 任务已花费时间，单位为秒 |
| TotalRecords   | uint64   | 集合总记录数 |
| ProcessedRecords | uint64 | 已处理的记录数 |


##示例##

1. 在集合 sample.employee 中异步创建索引

    ```lang-javascript
    > db.sample.employee.createIndexAsync('a', {a: 1})
    2328
    ```

2. 查看上述操作的任务信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_TASKS, {TaskID: 2328})
    ```

    输出结果如下：
    
    ```lang-json
    {
      "NodeName": "sdbserver1:11820",
      "TaskID": 2328,
      "Status": 9,
      "StatusDesc": "Finish",
      "TaskType": 2,
      "TaskTypeDesc": "Create index",
      "Name": "sample.employee",
      "IndexName": "a",
      "IndexDef": {
        "_id": {
          "$oid": "6098fe19820799d22f1f245e"
        },
        "UniqueID": 4037269258242,
        "key": {
          "a": 1
        },
        "name": "a"
      },
      "SortBufferSize": 64,
      "ResultCode": 0,
      "ResultCodeDesc": "Succeed",
      "ResultInfo": {},
      "OpInfo": "",
      "RetryCount": 0,
      "Progress": 100,
      "Speed": 0,
      "TimeSpent": 0.044396,
      "TimeLeft": 0,
      "TotalRecords": 0,
      "ProcessedRecords": 0
    }
    {
      "NodeName": "sdbserver1:11830",
      "TaskID": 2328,
      ...
    }
    ```

[^_^]:
     本文使用的所有引用和链接
[listTasks]:manual/Manual/Sequoiadb_Command/Sdb/listTasks.md
