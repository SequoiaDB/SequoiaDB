
##NAME##

getCpuInfo - Acquire CPU information

##SYNOPSIS##

***System.getCpuInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire CPU information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return CPU information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire CPU information

```lang-javascript
> System.getCpuInfo()
{
    "Cpus": [
      {
        "Core": 1,
        "Info": "Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz",
        "Freq": "2.19999814GHz"
      },
      {
        "Core": 1,
        "Info": "Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz",
        "Freq": "2.19999814GHz"
      }
    ],
    "User": 47223380,
    "Sys": 46662920,
    "Idle": 3513293040,
    "Other": 3023840
}
```