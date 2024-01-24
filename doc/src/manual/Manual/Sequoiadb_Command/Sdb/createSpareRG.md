##名称##

createSpareRG - 创建热备组

##语法##

**db.createSpareRG()**

##类别##

Sdb

##描述##

该函数用于创建热备组，用户可以通过该组管理[热备节点][spare]。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbReplicaGroup 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

创建一个热备组

```lang-javascript
> db.createSpareRG()
```



[^_^]:
    本文使用的所有引用及链接
[spare]:manual/Distributed_Engine/Maintainance/hot_spare.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md