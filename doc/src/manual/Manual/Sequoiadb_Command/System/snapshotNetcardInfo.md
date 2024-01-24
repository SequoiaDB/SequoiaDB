##名称##

snapshotNetcardInfo - 获取网卡的详细信息

##语法##

**System.snapshotNetcardInfo()**

##类别##

System

##描述##

获取网卡的详细信息

##参数##

无

##返回值##

返回网卡的详细信息

##错误##
如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

获取网卡的详细信息

```lang-javascript
> System.snapshotNetcardInfo()
{
    "CalendarTime": 1559722067,
    "Netcards": [
      {
        "Name": "lo",
        "RXBytes": 108885345140,
        "RXPackets": 97058303,
        "RXErrors": 0,
        "RXDrops": 0,
        "TXBytes": 108885345140,
        "TXPackets": 97058303,
        "TXErrors": 0,
        "TXDrops": 0
      },
      {
        "Name": "ens160",
        "RXBytes": 8267964446,
        "RXPackets": 6629177,
        "RXErrors": 0,
        "RXDrops": 141152,
        "TXBytes": 1864089945,
        "TXPackets": 2306206,
        "TXErrors": 0,
        "TXDrops": 0
      }
    ]
}
```