
##名称##

start - 启动 sdbcm 服务。

##语法##

**Oma.start( [options] )**

##类别##

Oma

##描述##

启动 sdbcm 服务。一般情况下，该接口被用于短暂启动一个临时的 sdbcm，以完成一些临时的任务。

##参数##

| 参数名   | 参数类型 | 默认值               | 描述                | 是否必填 |
| -------- | -------- | -------------------- | ------------------- | -------- |
| options  | JSON     | --- | 可选项，详见 options 选项说明。 | 否 |

options 参数详细说明如下：

| 属性 | 值类型 | 默认值 | 格式 | 描述 |
| ---- | ------ | ------ | ---- | ---- |
| port  | Int / String | 11790 | { port:11790 } | 指定启动 sdbcm 服务的端口 |
| alivetime  | Int / String | 300 | { alivetime:300 } | 服务存活时间，单位秒 |
| standalone  | Bool | false | { standalone:false } | 是否以独立模式启动 |

> Note：

> 1.  一台机器正常情况下只有一个 sdbcm 服务，但是可以通过 standalone 模式启动临时的 sdbcm 服务。

> 2. alivetime 参数仅在 standalone 为 true 时有效，并且 alivetime 结束时，临时的 sdbcm 服务会自动结束。


##返回值##

无返回值，出错抛异常，并输出错误信息。

##错误##


如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。
##版本##

v2.0及以上版本。

##示例##

1. 通过 standalone 模式启动临时的 sdbcm 服务，并指定端口为 11780，并且该临时 sdbcm 存活时间为 5分钟（300秒）。

	```lang-javascript
	> Oma.start({ port:11780,standalone:true })
    Success: sdbcm(11780) is successfully start (28741)
	```