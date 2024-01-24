##名称##

setDomain - 修改集合空间的所属域

##语法##

**db.collectionspace.setDomain(\<options\>)**

##类别##

SdbCS

##描述##

该函数用于修改集合空间的所属域。

##参数##

options ( *object，必填* )

通过 options 参数可以修改集合空间属性：

- Domain ( *string* )：所属域

    集合空间的数据必须分布在新指定域的组上

    格式：`Domain: <domain>`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.10 及以上版本

##示例##

创建一个集合空间，然后为该集合空间指定一个域

```lang-javascript
> db.createCS('sample')
> db.sample.setDomain({Domain: 'domain'})
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md