##名称##

getCL - 获取当前集合空间下指定的集合的对象引用

##语法##

**db.collectionspace.getCL(\<name\>)**

##类别##

SdbCS

##描述##

该函数用于获取当前集合空间下指定的集合的对象引用。

##参数##

name ( *string，必填* )

集合名

##返回值##

函数执行成功时，将返回指定集合的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getCL()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | 集合不存在| 检查集合是否存在|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.0 及以上版本

##示例##

返回集合空间 sample 下集合 employee 的引用

```lang-javascript
> var cl = db.sample.getCL("employee")
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md