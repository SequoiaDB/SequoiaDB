##名称##

updateNodeConfigs - 更新节点配置信息

##语法##

**oma.updateNodeConfigs(\<svcname\>, \<config\>)**

##类别##

Oma

##描述##

该函数用于更新指定节点的配置信息，更新后需重启节点或使用 [reloadConf()][reloadConf] 重载配置文件，使配置生效。


##参数##

- svcname ( *number/string，必填* )

	节点端口号

- config ( *object，必填* )

	节点配置信息，如更新日志大小、是否打开事务等，具体可参考[配置项参数][config]。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`updateNodeConfigs()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6     | SDB_INVALIDARG | 非法输入参数| 检查端口号和配置信息是否正确 |
| -259   | SDB_OUT_OF_BOUND | 未输入节点端口号或配置信息| 输入节点端口号以及配置信息 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2 及以上版本

##示例##

将节点 11810 的配置项参数 diaglevel 更新为 3

```lang-javascript
> var oma = new Oma("localhost", 11790)
> oma.updateNodeConfigs(11810, {diaglevel: 3})
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[reloadConf]:manual/Manual/Sequoiadb_Command/Sdb/reloadConf.md
[config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md