##名称##

current - 获取当前游标指向的记录

##语法##

**query.current()**

##类别##

SdbQuery

##描述##

获取当前游标指向的记录。

##参数##

无

##返回值##

返回当前游标指向的记录。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本。

##示例##

选择集合 employee 中 age 字段值大于 20 的记录，返回当前游标指向的记录。

```lang-javascript
> db.sample.employee.find( { age: { $gt: 20 } } ).current()
{
   "_id": {
     "$oid": "5cf8aef75e72aea111e82b38"
   },
   "name": "tom",
   "age": 20
}
```