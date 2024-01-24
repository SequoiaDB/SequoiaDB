##名称##

size - 返回当前游标到最终游标的记录条数

##语法##

**query.size()**

##类别##

SdbQuery

##描述##

返回当前游标到最终游标的记录条数。

> **Note:**

> query.size()返回的结果考虑 query.skip() 及 query.limit() 的影响。

##参数##

无

##返回值##

返回当前游标到最终游标的记录条数。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

选择集合 employee 下 age 大于10的记录（如使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询），返回当前游标到最终游标的记录条数。

```lang-javascript
> db.sample.employee.find( { age: { $gt: 20 } } ).size()
1
```