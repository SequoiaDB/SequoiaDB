
##名称##

createCoord - 创建临时协调节点

##语法##

**oma.createCoord(\<svcname\>, \<dbpath\>, [config])**

##类别##

Oma

##描述##

该函数用于在资源管理节点（sdbcm）所在的机器中创建一个临时协调节点，供初始创建 SequoiaDB 集群时使用。通过该函数创建的协调节点不会注册到编目节点中，即该节点不能被集群管理；如果希望协调节点能够被集群管理，可参考 [createNode()][createNode]。

##参数##

- svcname ( *string/number，必填* )

	节点端口号

- dbpath ( *string，必填* )

	节点的数据存储目录

- config ( *object，选填* )

	节点配置信息，如配置日志大小、是否打开事务等，具体可参考[数据库配置][configuration]

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createCoord()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -3     | SDB_PERM | 权限错误| 检查节点路径是否正确，路径权限是否正确 |
| -15    | SDB_NETWORK | 网络错误| 检查网络情况或 sdbcm 状态是否正常，如果状态异常，可以尝试重启|
| -145   | SDBCM_NODE_EXISTED | 节点已存在| 检查节点是否存在 |
| -157   | SDB_CM_CONFIG_CONFLICTS | 节点配置冲突 | 检查端口号及数据目录是否已经被使用 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

连接本地集群管理服务进程 sdbcm，创建一个端口号为 18800 的临时 coord 节点

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.createCoord(18800, "/opt/sequoiadb/database/coord/18800")
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[createNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md
    