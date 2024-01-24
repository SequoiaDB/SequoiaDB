##名称##

remove - 删除查询后的结果集

##语法##

**query.remove()**

##类别##

SdbQuery

##描述##

删除查询后的结果集。

> **Note:**  

> 1. 不能与 query.count() 、query.update() 同时使用。

> 2. 与 query.sort() 同时使用时，在单个节点上排序必须使用索引。 

> 3. 在集群中与 query.limit() 或 query.skip() 同时使用时，要保证查询条件会在单个节点或单个子表上执行。

##参数##

无

##返回值##

返回删除的结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

查询集合 employee 下 age 字段值大于 10 的记录，并将符合条件的记录删除。

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).remove()
{
   "_id": {
     "$oid": "5d2c4455f6d7aeedc15ddf87"
   },
   "name": "tom",
   "age": 18
}
```
