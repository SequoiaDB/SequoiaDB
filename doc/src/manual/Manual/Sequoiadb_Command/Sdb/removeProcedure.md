##名称##

removeProcedure - 删除指定的存储过程

##语法##

**db.removeProcedure(\<function name\>)**

##类别##

Sdb

##描述##

该函数用于删除指定的存储过程。该存储过程必须存在，否则出现异常信息。

##参数##

| 参数名 		| 类型   	| 描述 			| 是否必填 	|
| ------ 		| ------ 	| ------ 		| ------ 	|
| function name | string 	| 存储过程名 		| 是 		|

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`removeProcedure()`函数常见异常如下

| 错误码 		| 错误类型 | 可能发生的原因 	| 解决方法										|
| ------ 		| ------ 		| ------										| ------ |
| -233			| SDB_FMP_FUNC_NOT_EXIST | 存储过程不存在 | 使用命令 [listProcedures()][listProcedures] 确认指定存储过程是否存在	|
	
当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

删除名为“sum”的存储过程

```lang-javascript
> db.removeProcedure("sum")
```

[^_^]:
     本文使用的所有引用及链接
[listProcedures]:manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md