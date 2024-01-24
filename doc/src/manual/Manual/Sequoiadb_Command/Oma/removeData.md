
##名称##

removeData - 在目标集群控制器（sdbcm）所在的机器中删除指定的 standalone 节点。

##语法##

**oma.removeData(\<svcname\>)**

##类别##

Oma

##描述##

在目标集群控制器（sdbcm）所在的机器中删除指定的 standalone 节点。删除集群节点，请参考 [rg.removeNode()][removeNode]。

oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

##参数##

* `svcname` ( *Int32 | String*， *必填* )

	节点端口号。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`removeData()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -3     | SDB_PERM | 权限错误。| 检查节点路径是否正确，路径权限是否正确。 |
| -15    | SDB_NETWORK | 网络错误。| 1. 检查 sdbcm 状态是否正常，如果状态异常，可以尝试重启。<br> 2. 检查网络情况。 |
| -146   | SDBCM_NODE_NOTEXISTED | 节点不存在。| 检查节点是否存在。 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。 可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v2.0 及以上版本。

##示例##

1. 在本地删除一个端口号为11820的 standalone 节点

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.removeData( 11820 )
	```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[removeNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/removeNode.md
