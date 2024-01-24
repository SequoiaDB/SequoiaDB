##名称##

dropIdIndex - 删除集合中的 $id 索引

##语法##

**db.collectionspace.collection.dropIdIndex\(\)**

##类别##

SdbCollection

##描述##

该函数用于删除集合中的 $id 索引，同时禁止更新或删除操作。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛出异常并输出错误信息。

##错误##

`dropIdIndex()` 函数常见异常如下：

|错误码  | 错误码类型 | 可能发生的原因    |  解决办法 |
| ------ | ---------- |------------- |  -------- |
| -47    | SDB_IXM_NOTEXIST |$id 索引不存在 |  -        |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

删除集合中的 $id 索引

```lang-javascript
> db.sample.employee.dropIdIndex()
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md