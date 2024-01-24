##名称##

current - 获取当前游标指向的记录

##语法##

**cursor.current()**

##类别##

SdbCursor

##描述##

该函数用于获取当前游标指向的记录，更多内容可查看 [next()][next] 方法。

##参数##

无

##返回值##

函数执行成功时，如果游标有记录返回，返回值为 BSONObj 类型的对象；否则，返回值为 null 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

| 错误码 | 错误类型 |可能发生的原因 	| 解决办法					|
| ------ | ------ 	| -----------	| ------					|
| -29    | SDB_DMS_EOC | 当服务端没有记录返回时，如果通过游标的 current() 接口去获取第一条记录，该接口将调用失败 | 在任何情况下，用户都应该先使用游标的 next() 接口去获取第一条记录。当服务端没有记录返回时，next() 接口将返回 null，而不是抛出 -29 错误 |
| -31	 | SDB_DMS_CONTEXT_IS_CLOSE | 上下文已关闭 | 确认查询记录是否为0条	|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.0 及以上版本

##示例##

选择集合 employee 下 a 为 1 的记录，返回当前游标指向的记录

```lang-javascript
> var cur = db.sample.employee.find({a: 1});
> var obj = null;
> while((obj = cur.next() != null)){
    println("Record is:" + cur.current());
}
Record is:{
"_id": {
    "$oid": "60470a4db354306ff89cd355"
},
"a": 1
}
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[next]:manual/Manual/Sequoiadb_Command/SdbCursor/next.md