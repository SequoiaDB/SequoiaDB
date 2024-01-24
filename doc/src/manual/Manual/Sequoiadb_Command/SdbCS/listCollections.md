##名称##

listCollections - 列举集合空间所包含的集合名称信息

##语法##

**db.collectionspace.listCollections()**

##类别##

SdbCS

##描述##

该函数用于列举集合空间中所有的集合名称信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.2 及以上版本

##示例##
列举集合空间 sample 所包含的集合名称信息

```lang-javascript
> db.sample.listCollections()
{
  "Name": "sample.a"
}
{
  "Name": "sample.b"
}
{
  "Name": "sample.employee"
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md