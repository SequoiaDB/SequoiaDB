##名称##

arrayAccess - 将结果集保存到数组中并获取指定下标记录

##语法##

**query.arrayAccess( \<index\> )**

##类别##

SdbQuery

##描述##

先将结果集保存到数组中，然后获取指定下标的记录，下标从 0 开始。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述               | 是否必填 |
| ------ | -------- | ------ | ------------------ | -------- |
| index  | int      | ---    | 要访问的记录的下标 | 是       |

##返回值##

返回指定下标的记录。

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。
关于错误处理可以参考[常见错误处理指南][faq]。

常见错误可参考[错误码][error_code]。

##版本##

v3.0 及以上版本。

##示例##

返回数组中下标为 0 的记录

```lang-javascript
> db.sample.employee.find().arrayAccess(0)
{
    "_id": {
      "$oid": "5cf8aef75e72aea111e82b38"
    },
    "name": "tom",
    "age": 20
}
```


[^_^]:
     本文使用的所有引用及链接
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md