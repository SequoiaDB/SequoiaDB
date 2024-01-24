##名称##

limit - 控制查询返回的记录条数

##语法##

**query.limit(\<num\>)**

##类别##

SdbQuery

##描述##

该函数用于控制查询返回的记录条数。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| num    | number | 自定义返回结果集的记录条数| 是 |

##返回值##

函数执行成功时，将返回一个 SdbQuery 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

获取集合 sample.employee 中字段 age 最大的记录

```lang-javascript
> db.sample.employee.find().sort({age: -1}).limit(1)
{
  "_id": {
    "$oid": "5813035cc842af52b6000009"
  },
  "name": "Tom",
  "age": 22
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md