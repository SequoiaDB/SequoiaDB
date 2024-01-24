##名称##

Remote - 新建一个远程连接对象

##语法##

***var remote = new Remote( [hostname], [svcname] )***

##类别##

Remote

##描述##

新建一个远程连接对象。

##参数##

| 参数名   | 参数类型 | 默认值            | 描述         | 是否必填 |
| -------- | -------- | ----------------- | ------------ | -------- |
| hostname | string   | localhost         | 主机 IP 地址 | 否       |
| svcname  | int      | 本地 sdbcm 的端口 | sdbcm 的端口 | 否       |

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。


常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

新建一个远程连接对象

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```