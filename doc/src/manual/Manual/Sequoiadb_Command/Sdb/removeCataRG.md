##名称##

removeCataRG - 删除编目复制组

##语法##

**db.removeCataRG()**

##类别##

Sdb

##描述##

该函数用于删除编目复制组。该操作会删除复制组中所有编目节点，因此目标复制组中不能存在数据节点及协调节点的信息。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.10 及以上版本

##示例##

删除编目分区组

```lang-javascript
> db.removeCataRG()
```


[^_^]:
    本文使用的所有链接及引用
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md