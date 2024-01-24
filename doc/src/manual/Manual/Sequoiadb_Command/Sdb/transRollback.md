##名称##

transRollback - 事务回滚

##语法##

**db.transRollback()**

##类别##

Sdb

##描述##

该函数用于事务回滚。在开启事务之后，如果单个逻辑工作单元执行的操作出现异常，执行事务回滚命令，那么数据库回到原来状态。


##参数##

无


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

执行事务回滚命令

```lang-javascript
> db.transRollback()
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md