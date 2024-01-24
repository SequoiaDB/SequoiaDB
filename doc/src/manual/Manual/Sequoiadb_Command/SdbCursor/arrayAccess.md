##名称##

arrayAccess - 将结果集保存到数组中并获取指定下标的记录

##语法##

**cursor.arrayAccess(\<index\>)**

**cursor[\<index\>]**

##类别##

SdbCursor

##描述##

该函数用于从保存结果集的数组中，获取指定下标的记录。

##参数##

`index` ( *number，必填* )

要访问的记录的下标

##返回值##

函数执行成功时，将返回一条 String 类型的记录。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.0 及以上版本

##示例##

返回数组中下标为 0 的记录

```lang-javascript
> db.sample.employee.find().arrayAccess(0)
{
   "_id": {
   "$oid": "581192bd6db4da2a23000009"
   },
   "a": 9
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md