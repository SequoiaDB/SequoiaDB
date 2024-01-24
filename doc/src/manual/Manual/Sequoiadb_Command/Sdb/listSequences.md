##名称##

listSequences - 枚举序列信息

##语法##

**db.listSequences()**

##类别##

Sdb

##描述##

该函数用于枚举当前数据库的序列信息。

##参数##

无

##返回值##

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看 [$LIST_SEQUENCES][LIST_SEQUENCES]

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.2 及以上版本

##示例##

查看当前数据库的序列信息。

```lang-javascript
> db.listSequences()
{
    "Name": "SYS_8589934593_id_SEQ"
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[LIST_SEQUENCES]:manual/Manual/SQL_Grammar/Monitoring/LIST_SEQUENCES.md