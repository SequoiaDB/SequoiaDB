##名称##

listCollections - 枚举域中的集合信息

##语法##

**domain.listCollections()**

##类别##

SdbDomain

##描述##

该函数用于枚举指定域中的全部集合信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取域中的集合信息，字段说明可参考[集合列表][collections_list]。

函数执行失败时，将抛异常并输出错误信息。


##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

获取指定域下的集合

```lang-javascript
> domain.listCollections()
{
    "Name": "sample.employee" 
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[collections_list]:manual/Manual/List/SDB_LIST_COLLECTIONS.md