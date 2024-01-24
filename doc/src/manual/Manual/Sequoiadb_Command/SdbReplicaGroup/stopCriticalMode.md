##名称##

stopCriticalMode - 在当前复制组中停止 Critical 模式

##语法##

**rg.stopCriticalMode()**

##类别##

SdbReplicaGroup 

##描述##

该函数用于在当前复制组中停止 Critical 模式。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

在复制组 group1 中停止 Critical 模式

```lang-javascript
> var rg = db.getRG("group1")
> rg.stopCriticalMode()
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
