
##名称##

createOM - 在目标集群控制器（sdbcm）所在的机器中创建 sdbom 服务进程（ SequoiaDB 管理中心进程）。

##语法##

**oma.createOM(\<svcname\>,\<dbpath\>,[config])**

##类别##

Oma

##描述##

在目标集群控制器（sdbcm）所在的机器中创建sdbom服务进程（ SequoiaDB 管理中心进程）。

**Note:**

* oma 对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

* 一个集群只能归属于一个SequoiaDB 管理中心管理，但一个 SequoiaDB 管理中心却可管理多个集群。一般只创建一个 sdbom 服务进程即可。

##参数##

* `svcname` ( *Int | String*， *必填* )

    节点端口号。

* `dbpath` ( *String*， *必填* )

    节点路径。

* `config` ( *Object*， *选填* )

	节点配置信息，如配置日志大小等，具体可参考[数据库配置][configuration_parameters]。

   | 常用配置 | 描述 | 默认值 |
   | -------- | ---- | ------ |
   | httpname | 设置sdbom的网页端口 | svcname + 4 |
   | wwwpath  | 设置sdbom的网页路径 | sequoiadb安装路径的web目录 |

##返回值##

成功：返回 Oma 对象。

失败：抛出异常。

##错误##

`createOM()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误      | 确认参数类型和参数个数是否正确	|
| -145 | SDBCM_NODE_EXISTED | 节点已存在    | 使用列表查看节点是否存在	|

当异常抛出时，用户可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息，或通过 [getLastError()][getLastError] 获取[错误码][error_code]。关于错误处理可以参考[常见错误处理指南][faq]


##版本##

v2.0 及以上版本。

##示例##

1. 在本地中创建并启动一个本地端口号为11780，http端口为8000，web路径为/opt/sequoiadb/web的sdbom进程

	```lang-javascript
	> var oma = new Oma("localhost", 11790)
	> oma.createOM( "11780", "/opt/sequoiadb/database/sms/11780",
                             { "httpname": 8000, "wwwpath": "/opt/sequoiadb/web" } )
	> oma.startNode( 11780 )
 	```



[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[configuration_parameters]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md