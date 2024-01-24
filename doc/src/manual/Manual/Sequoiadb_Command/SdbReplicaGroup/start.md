##名称##

start - 启动当前复制组

##语法##

**rg.start()**

##类别##

SdbReplicaGroup

##描述##

该函数用于启动当前复制组。复制组启动之后才能创建节点及其他操作。也可以使用方法 [startRG()][startRG] 启动指定的节点。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

启动复制组 group1 

```lang-javascript
> var rg = db.getRG("group1")
> rg.start()   //等价于 db.startRG("group")
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[startRG]:manual/Manual/Sequoiadb_Command/Sdb/startRG.md