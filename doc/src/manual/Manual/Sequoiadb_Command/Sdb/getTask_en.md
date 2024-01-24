##NAME##

getTask - get information about the specified task

##SYNOPSIS##

**db.getTask(\<id\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get task information of specified task ID.

##PARAMETERS##

id ( *number, required* )

Specify task ID.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. For field descriptions, refer to [SYSTASKS][SYSTASKS].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code][error_code]. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.6 and above

##EXAMPLES##

1.  Create the index asynchronously.

    ```lang-javascript
    > db.sample.employee.createIndexAsync("a", {a: 1})
    1051
    ```

2. Get the task information.

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