
##名称##

getNodeConfigs - 从配置文件中获取指定端口的数据库节点的配置信息。

##语法##

**oma.getNodeConfigs(\<svcname\>)**

##类别##

Oma

##描述##

从配置文件中获取指定端口的数据库节点的配置信息。

##参数##

* `svcname` ( *Int | String*， *必填* )

	节点端口号。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`getNodeConfigs()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -4     | SDB_FNE| 文件不存在。| 检查节点的配置文件是否存在。 |


当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##版本##

v2.0及以上版本。

##示例##

获取目标机器 sdbserver1 上的 11820 节点的配置信息。

```lang-javascript
> var oma = new Oma( "sdbserver1", 11790 )
> oma.getNodeConfigs( 11820 )
{
"catalogaddr": "sdbserver1:11803",
"dbpath": "/opt/sequoiadb/database/data/11820/",
"diaglevel": "5",
"role": "data",
"svcname": "11820"
}
```