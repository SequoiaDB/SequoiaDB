##名称##

getReleaseInfo - 获取操作系统发行版本信息

##语法##

**System.getReleaseInfo()**

##类别##

System

##描述##

获取操作系统发行版本信息

##参数##

无

##返回值##

返回操作系统的发行版本信息

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

获取操作系统的发行版本信息

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