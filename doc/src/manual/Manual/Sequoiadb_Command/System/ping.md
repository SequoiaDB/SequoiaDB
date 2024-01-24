##名称##

ping - 判断到达指定主机的网络是否连通

##语法##

**System.ping( \<hostname\> )**

##类别##

System

##描述##

判断到达指定主机的网络是否连通

##参数##

| 参数名    | 参数类型 | 默认值 | 描述         | 是否必填 |
| --------- | -------- | ------ | ------------ | -------- |
| hostname  | string   | ---    | 主机名称     | 是       |

##返回值##

如果到达指定主机的网络能连通则返回 true，否则返回 false

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

测试到达一个主机的网络是否连通

```lang-javascript
> System.ping( "hostname" )
{
    "Target": "hostname",
    "Reachable": true
}
```