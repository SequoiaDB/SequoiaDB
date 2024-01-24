##名称##

createNode - 在当前复制组中创建节点

##语法##

**rg.createNode(\<host\>, \<service\>, \<dbpath\>, \[config])**

##类别##

SdbReplicaGroup

##描述##

该函数用于在当前复制组中创建节点。

##参数##

- host（ *string，必填* ）

    主机名

- service（ *number，必填* ）

    节点端口号

- dbpath（ *string，必填* ）

    节点数据文件的存储路径

    >**Note:**
    >
    > 数据库管理用户（安装 SequoiaDB 时创建，默认为 sdbadmin）需拥有该参数指定目录的写权限。

- config（ *object，选填* ）

    节点配置信息，如配置日志大小、是否打开事务等，具体配置可参考[参数说明][cluster_config]

##返回值##

函数执行成功时，将返回一个 SdbNode 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createNode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -15      |SDB_NETWORK| 网络错误     | 1）检查 sdbcm 状态是否正常，如果状态异常，可以尝试重启 sdbcm<br> 2）检查“主机名/IP”映射关系是否正确，网络是否能正常通信 |
| -145     |SDBCM_NODE_EXISTED| 节点已存在   | 检查节点是否存在 |
| -157     |SDB_CM_CONFIG_CONFLICTS| 节点配置冲突 | 检查节点端口是否被占用 |
| -3       | SDB_PERM|权限错误     | 检查节点路径和路径权限是否正确 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

在复制组 group1 中创建节点"sdbserver1:11830"，并指定同步日志文件大小为 64MB

```lang-javascript
> var rg = db.getRG("group1")
> rg.createNode("sdbserver1", 11830, "/opt/sequoiadb/database/data/11830", {logfilesz: 64})
```

> **Note:**  
>
> 一个复制组中能创建多个节点，每个节点需要预留至少五个顺延的端口。因为系统为每个节点后台控制了五个通信接口。


[^_^]:
    本文使用的所有引用及链接
[cluster_config]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
