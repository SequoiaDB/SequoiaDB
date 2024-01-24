
##NAME##

getReleaseInfo - Acquire the information of operating system release

##SYNOPSIS##

***System.getReleaseInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire the information of operating system release

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the information of operating system release.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire the information of operating system release

```lang-javascript
> System.getReleaseInfo()
{
  "Distributor": "Ubuntu",
  "Release": "16.04",
  "Description": "Ubuntu 16.04.6 LTS",
  "KernelRelease": "4.4.0-116-generic",
  "Bit": 64
}
```