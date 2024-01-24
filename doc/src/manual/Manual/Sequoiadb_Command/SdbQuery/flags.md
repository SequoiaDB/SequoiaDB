##名称##

flags - 按指定的标志位遍历结果集

##语法##

**query.flags( \<flag\> )**

##类别##

SdbQuery

##描述##

按指定的标志位遍历结果集。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述   | 是否必填 |
| ------ | -------- | ------ | ------ | -------- |
| flag   | 枚举     | ---    | 标志位 | 是       |

flag 参数的可选值如下表：

| 可选值 | 描述                         |
| ------ | ---------------------------- |
| SDB_FLG_QUERY_FORCE_HINT | 当添加该标志位之后，将强制使用指定的索引进行查询，如果数据库没有指定的索引，则报错，无法查询 |
| SDB_FLG_QUERY_PARALLED | 当添加该标志位之后，将启用并行子查询，每个子查询将完成扫描不同部分的数据 |
| SDB_FLG_QUERY_WITH_RETURNDATA | 一般查询时不会返回数据，只返回相应的游标，然后再通过游标获取数据。当添加该标志位之后，将在查询响应中返回数据，该标志位是默认开启的 |
| SDB_FLG_QUERY_PREPARE_MORE | 在查询时，服务端会与客户端进行多次传输，把查询结果返回给客户端。当添加该标志位之后，服务端每次会传输更多的查询数据给客户端，减少服务端和客户端的传输次数，从而减少网络开销 |
| SDB_FLG_QUERY_FOR_UPDATE | 对查询取得的记录加U锁。当会话在事务中时，且添加该标志位之后，事务锁将不会在事务提交或回滚之前释放。当会话不在事务中时，该标志位不起作用 |
| SDB_FLG_QUERY_FOR_SHARE | 对查询取得的记录加S锁。当会话在事务中时，且添加该标志位之后，事务锁将不会在事务提交或回滚之前释放。当会话不在事务中时，该标志位不起作用 |


> **Note:**

> 假设用户指定了集合中不存在的索引查询数据，那么查询会进行全表查询，而不是索引查询。如果用户指定了集合中不存在的索引查询数据，然后又指定了 SDB_FLG_QUERY_FORCE_HINT 标志位，因为该标志位是强制进行索引查询，所以用户执行该操作会报错。 

##返回值##

返回查询结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本。

##示例##

强制使用指定的索引进行查询。

```lang-javascript
> db.sample.employee.find().hint( { "": "ageIndex" } ).flags( SDB_FLG_QUERY_FORCE_HINT )
{
   "_id": {
     "$oid": "5d412cfa614afb5557b2b41d"
   },
   "name": "fang",
   "age": 18
}
{
   "_id": {
     "$oid": "5d412cfa614afb5557b2b41c"
   },
   "name": "alice",
   "age": 19
}
{
   "_id": {
     "$oid": "5d412cfa614afb5557b2b41b"
   },
   "name": "ben",
   "age": 21
}

> db.sample.employee.find().hint( { "": "notExistIndex" } ).flags( SDB_FLG_QUERY_FORCE_HINT )
uncaught exception: -53
Invalid hint
```
