
##名称##

setNodeConfigs - 对指定端口的数据库节点，用新的节点配置信息覆盖该节点原来配置文件上的配置信息。

##语法##

**oma.setNodeConfigs(\<svcname\>,\<config\>)**

##类别##

Oma

##描述##

对指定端口的数据库节点，用新的节点配置信息覆盖该节点原来配置文件上的配置信息。

**Note:**

* 覆盖后，配置文件中原来的内容会丢失。
* 使用 [reloadConf()](manual/Manual/Sequoiadb_Command/Sdb/reloadConf.md) 重载配置文件。
* 使用 [updateConf()](manual/Manual/Sequoiadb_Command/Sdb/updateConf.md) 和 [deleteConf()] (manual/Manual/Sequoiadb_Command/Sdb/deleteConf.md) 在线修改配置。

##参数##

* `svcname` ( *Int | String*， *必填* )

	节点端口号。

* `config` ( *Object*， *必填* )

	节点配置信息，如配置日志大小，是否打开事务等，具体可参考[数据库配置](manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md)。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`setNodeConfigs()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6     | SDB_INVALIDARG | 非法输入参数。| 检查端口号和配置信息是否正确。 |
| -259   | SDB_OUT_OF_BOUND | 未输入节点端口号或配置信息。| 输入节点端口号以及配置信息。 |

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##示例##

1. 用新配置覆盖端口号为 11810 的节点的配置。

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.setNodeConfigs( 11810, { svcname: "11810", dbpath: "/home/users/sequoiadb/trunk/11810", diaglevel: 3, clustername: "xxx", businessname: "yyy", role: "data", catalogaddr: "ubuntu1:11823, ubuntu2:11823" } )
    ```