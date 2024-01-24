
##NAME##

getHostName - Acquire the hostname

##SYNOPSIS##

***System.getHostName()***

##CATEGORY##

System

##DESCRIPTION##

Acquire the hostname

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return hostname.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire the hostname

```lang-javascript
> System.getHostName()
hostname
```