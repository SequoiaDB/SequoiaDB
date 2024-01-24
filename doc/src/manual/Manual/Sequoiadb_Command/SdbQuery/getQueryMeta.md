##名称##

getQueryMeta - 获取查询元数据信息

##语法##

**query.getQueryMeta()**

##类别##

SdbQuery

##描述##

获取查询元数据信息。

##参数##

无

##返回值##

返回查询元数据信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.0 及以上版本。

##示例##

获取查询元数据信息

```lang-javascript
> db.sample.employee.find().getQueryMeta()
{
    "HostName": "ubuntu",
    "ServiceName": "42000",
    "NodeID": [
      1001,
      1003
    ],
    "ScanType": "tbscan",
    "Datablocks": [
      9
    ]
}
```