##名称##

getTask - 获取指定任务的信息

##语法##

**db.getTask(\<id\>)**

##类别##

Sdb

##描述##

该函数用于获取指定任务 ID 的任务信息。

##参数##

id（ *number，必填* ）

指定任务 ID

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取任务详细信息，字段说明可参考 [SYSTASKS 集合][SYSTASKS]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.6 及以上版本

##示例##

1. 异步创建索引

    ```lang-javascript
    > db.sample.employee.createIndexAsync("a", {a: 1})
    1051
    ```

2. 查看相应的任务信息

    ```lang-javascript
    > db.getTask(1051)
    {
      "BeginTimestamp": "2021-06-01-15.50.23.201403",
      "CreateTimestamp": "2021-06-01-15.50.22.315086",
      "EndTimestamp": "2021-06-01-15.50.23.206842",
      "FailedGroups": 0,
      "Groups": [
        {
          "GroupName": "group1",
          "OpInfo": "",
          "ProcessedRecords": 0,
          "Progress": 100,
          "ResultCode": 0,
          "ResultCodeDesc": "Succeed",
          "ResultInfo": {},
          "RetryCount": 0,
          "Speed": 0,
          "Status": 9,
          "StatusDesc": "Finish",
          "TimeLeft": 0,
          "TimeSpent": 0.003873,
          "TotalRecords": 0
        }
      ],
      "IndexDef": {
        "_id": {
          "$oid": "60b5e6be5777c1ae52445985"
        },
        "UniqueID": 4294967305,
        "key": {
          "a": 1
        },
        "name": "a"
      },
      "IndexName": "a",
      "Name": "sample.employee",
      "Progress": 100,
      "ResultCode": 0,
      "ResultCodeDesc": "Succeed",
      "ResultInfo": {},
      "SortBufferSize": 64,
      "Speed": 0,
      "Status": 9,
      "StatusDesc": "Finish",
      "SucceededGroups": 1,
      "TaskID": 1051,
      "TaskType": 2,
      "TaskTypeDesc": "Create index",
      "TimeLeft": 0,
      "TimeSpent": 0.005439,
      "TotalGroups": 1,
      "_id": {
        "$oid": "60b5e6be5777c1ae52445986"
      }
    }
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[SYSTASKS]:manual/Manual/Catalog_Table/SYSTASKS.md
[error_code]:manual/Manual/Sequoiadb_error_code.md