##名称##

removeDomain - 移除集合空间的所属域

##语法##

**db.collectionspace.removeDomain()**

##类别##

SdbCS

##描述##

该函数用于移除集合空间的所属域。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.10 及以上版本

##示例##

创建一个集合空间并指定一个域，然后把该集合空间移除该域

```lang-javascript
> db.createCS('sample', {Domain: 'domain'})
> db.sample.removeDomain()
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md