##名称##

dropAll - 删除所有回收站项目

##语法##

**db.getRecycleBin().dropAll([options])**

##类别##

SdbRecycleBin

##描述##

该函数用于删除所有回收站项目。

##参数##

options（ *object，选填* )

通过 options 可以设置其他选填参数：

- Async（ *boolean* ）：是否使用异步模式删除回收站项目，默认为 false，表示不使用异步模式

    格式：`Async: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

删除所有回收站项目

```lang-javascript
> db.getRecycleBin().dropAll()
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

