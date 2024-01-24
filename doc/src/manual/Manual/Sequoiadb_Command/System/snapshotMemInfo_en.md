
##NAME##

snapshotMemInfo - Acquire memory information

##SYNOPSIS##

***System.snapshotMemInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire memory information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return memory information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire memory information

```lang-javascript
> System.snapshotMemInfo()
{
  "Size": 5967,
  "Used": 2919,
  "Free": 384,
  "Unit": "M"
}
```