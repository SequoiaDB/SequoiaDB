
##名称##

createData - 在目标集群控制器（sdbcm）所在的机器中创建一个 standalone 节点。

##语法##

**oma.createData(\<svcname\>,\<dbpath\>,[config])**

##类别##

Oma

##描述##

在目标集群控制器（sdbcm）所在的机器中创建一个 standalone 节点。

**Note:**

* oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

##参数##

* `svcname` ( *Int | String*， *必填* )

	节点端口号。

* `dbpath` ( *String*， *必填* )

	节点数据目录。

* `config` ( *Object*， *选填* )

	节点配置信息，如配置日志大小，是否打开事务等，具体可参考[数据库配置][cluster_config]。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`createData()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -3     | SDB_PERM | 权限错误。| 检查节点路径是否正确，路径权限是否正确。 |
| -15    | SDB_NETWORK | 网络错误。| 1. 检查 sdbcm 状态是否正常，如果状态异常，可以尝试重启。2. 检查网络情况。 |
| -145   | SDBCM_NODE_EXISTED | 节点已存在。| 检查节点是否存在。 |
| -157   | SDB_CM_CONFIG_CONFLICTS | 节点配置冲突。 | 检查端口号及数据目录是否已经被使用。 |

当异常抛出时，用户可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息，或通过 [getLastError()][getLastError] 获取错误码。关于错误处理可以参考[常见错误处理指南][faq]。

常见错误可参考[错误码][error_code]。

##版本##

v2.0 及以上版本。

##示例##

1. 在本地创建一个端口号为11820的 standalone 节点，指定日志文件大小为64MB。

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.createData( 11820, "/opt/sequoiadb/standlone/11820", { logfilesz: 64 } )
	```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[cluster_config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md