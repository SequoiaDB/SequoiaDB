##名称##

connect - 获取当前节点的连接

##语法##

**node.connect([useSSL])**

##类别##

SdbNode

##描述##

获取当前节点的连接，进而对当前节点进行一系列的操作。可以使用 node.connect().help() 查看相关的操作

##参数##

* useSSL ( *boolean*， *选填* )

  是否启用 SSL 连接，默认为 false。

> **Note:**

> 1. 目前只有企业版支持 SSL 功能，所以 useSSL 为 true 仅对企业版有效。

> 2. 启用 SSL 连接之前需要先设置数据库配置项 --usessl=true ，请参考[配置项参数][CONF_PARAMETERS]。

##返回值##

函数执行成功：返回 [Sdb][Sdb] 对象。

函数执行失败：将抛异常并输出错误信息。

##错误##

`connect()`函数常见异常如下：

| 错误码 		| 可能的原因 	| 解决方法					|
| ------ 		| ------ 		| ------					|
| -15			| 网络错误      | 检查语法，查看节点是否启动|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.10及以上版本

##示例##

* 获取当前节点的普通连接

```lang-javascript
> node.connect()
localhost:11820
```

* 获取当前节点的 SSL 连接

```lang-javascript
> node.connect(true)
localhost:11820
```

[^_^]:
     本文使用的所有引用及链接
[CONF_PARAMETERS]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[Sdb]:manual/Manual/Sequoiadb_Command/Sdb/Sdb.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md