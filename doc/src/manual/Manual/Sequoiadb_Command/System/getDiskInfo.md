##名称##

getDiskInfo - 获取磁盘的信息

##语法##

**System.getDiskInfo()**

##类别##

System

##描述##

获取磁盘的信息

##参数##

无

##返回值##

返回磁盘的信息

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

获取磁盘的信息

```lang-javascript
> System.getDiskInfo()
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
      ...
    ]
}
```