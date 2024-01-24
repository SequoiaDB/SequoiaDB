##名称##

getServiceName - 获取节点的服务器名

##语法##

**node.getServiceName()**

##类别##

SdbNode

##描述##

该函数用于获取节点的服务器名。

##参数##

无

##返回值##

函数执行成功时，将返回一个 String 类型的对象。通过该对象获取节点的服务器名。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

获取 node 节点的服务器名

```lang-javascript
> node.getServiceName()
11800
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
