##名称##

stopAllNodes - 停止指定业务名的所有节点

##语法##

**oma.stopAllNodes(\[businessName\])**

##类别##

Oma

##描述##

该函数用于在资源管理节点（sdbcm）所在的机器中，停止指定业务名的所有节点。

##参数##

businessName ( *string，选填* )

业务名

- 该参数仅内部使用
- 如果不指定该参数，则默认停止 sdbcm 所在机器的所有节点

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

停止所有业务名为"yyy"的节点

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.stopAllNodes( "yyy" )
Stop sequoiadb(30000): Success
Stop sequoiadb(30010): Success
Stop sequoiadb(30020): Success
Stop sequoiadb(20000): Success
Stop sequoiadb(40000): Success
Stop sequoiadb(41000): Success
Stop sequoiadb(42000): Success
Stop sequoiadb(50000): Success
Total: 8; Success: 8; Failed: 0
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md