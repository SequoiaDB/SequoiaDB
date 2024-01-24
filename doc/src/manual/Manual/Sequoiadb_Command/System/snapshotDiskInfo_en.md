
##NAME##

snapshotDiskInfo - Acquire disk information

##SYNOPSIS##

***System.snapshotDiskInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire disk information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return disk information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire disk information

```lang-javascript
> System.snapshotDiskInfo()
{
  "Disks": [
    {
      "Filesystem": "udev",
      "FsType": "devtmpfs",
      "Size": 2963,
      "Used": 0,
      "Unit": "MB",
      "Mount": "/dev",
      "IsLocal": false,
      "ReadSec": 0,
      "WriteSec": 0
    },
    {
      "Filesystem": "tmpfs",
      "FsType": "tmpfs",
      "Size": 596,
      "Used": 60,
      "Unit": "MB",
      "Mount": "/run",
      "IsLocal": false,
      "ReadSec": 0,
      "WriteSec": 0
    },
  ]
}
```