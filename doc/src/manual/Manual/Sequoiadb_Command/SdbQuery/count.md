##名称##

count - 查询符合匹配条件的记录条数

##语法##

**query.count()**

##类别##

SdbQuery

##描述##

查询符合匹配条件的记录条数。

> **Note:** 

> query.count() 返回的结果忽略 [query.skip()](manual/Manual/Sequoiadb_Command/SdbQuery/skip.md) 及 [query.limit()](manual/Manual/Sequoiadb_Command/SdbQuery/limit.md) 的影响。

##参数##

无

##返回值##

返回符合匹配条件的记录条数。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

选择集合 employee 下 age 大于10的记录，使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询返回符合匹配条件 { age: { $gt: 10 } } 的记录条数。

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).count()
3
```
