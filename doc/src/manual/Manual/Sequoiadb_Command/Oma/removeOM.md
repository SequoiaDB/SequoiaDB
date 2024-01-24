
##名称##

removeOM - 在目标集群控制器（sdbcm）所在的机器中删除 sdbom 服务进程（SequoiaDB 管理中心进程）。

##语法##

**oma.removeOM(\<svcname\>)**

##类别##

Oma

##描述##

在目标集群控制器（sdbcm）所在的机器中删除 sdbom 服务进程（SequoiaDB 管理中心进程）。

**Note:**

* oma 对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

##参数##

* `svcname` ( *Int | String*， *必填* )

    节点端口号。

##返回值##

成功：无。

失败：抛出异常。

##错误##

`removeOM()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -146   | SDBCM_NODE_NOTEXISTED | 节点不存在。| 检查节点是否存在。 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。 可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v2.0 及以上版本。

##示例##

1. 删除安装在本地的 sdbom 服务进程

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.removeOM( 11780 )
	```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md