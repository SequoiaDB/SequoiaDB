
##名称##

SecureSdb - SecureSdb 对象

##语法##

***var securesdb = new SecureSdb( [hostname], [svcname] )***

***var securesdb = new SecureSdb( [hostname], [svcname], [username], [password] )***

##类别##

SecureSdb

##描述##

该函数用于新建一个 SecureSdb 对象。

> **Note:**

> - SecureSdb 是 Sdb 的子类，SecureSdb 的对象使用 SSL 连接，目前只有企业版支持 SSL 功能。

> - 在使用 SecureSdb 之前需要先设置数据库配置项 --usessl=true ，可参考[配置项参数](manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md)。

> - SecureSdb 对象和 Sdb 对象的方法和语法一致。 

##参数##

| 参数名   | 类型 | 描述         | 是否必填 |
| -------- | -------- | ------------ | -------- |
| hostname | string   | 主机 IP 地址，默认为“localhost” | 否     |
| svcname  | number      | coord 的端口，默认为本地 coord 的端口 | 否     |
| username  | string      | 用户名，默认为"" | 否     |
| password  | string      | 密码，默认为"" | 否     |

##返回值##

函数执行成功时，返回 SecureSdb 对象。 

函数执行失败时，将抛出异常并输出错误信息。


##错误##


当异常抛出时，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本

##示例##

新建一个 SecureSdb 对象

```lang-javascript
> var securesdb = new SecureSdb( "192.168.20.71", 11810 )
```