
##NAME##

snapshotCpuInfo - Acquire basic CPU information

##SYNOPSIS##

***System.snapshotCpuInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire basic CPU information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return basic CPU information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire basic CPU information

```lang-javascript
> System.snapshotCpuInfo()
{
  "User": 47223380,
  "Sys": 46662920,
  "Idle": 3513293040,
  "Other": 3023840
}
```