##名称##

listProcedures - 枚举存储过程

##语法##

**db.listProcedures( [cond] )**

##类别##

Sdb

##描述##

该函数用于枚举所有的存储过程函数。

##参数##

| 参数名 | 参数类型  | 描述 													  | 是否必填 |
|--------|-----------| -----------------------------------------------------------|----------|
| cond 	 | Json 对象 | 条件为空时，枚举所有的函数，不为空时，枚举符合条件的函数。 | 否	   	 |


##返回值##

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看 [STOREPROCEDURES 集合][STOREPROCEDURES]

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 列出所有的函数信息

	```lang-javascript
	> db.listProcedures()
	{ "_id" : { "$oid" : "52480389f5ce8d5817c4c353" }, 
	  "name" : "sum", 
	  "func" : "function sum(x, y) {return x + y;}", 
	  "funcType" : 0 
	}
	{ "_id" : { "$oid" : "52480d3ef5ce8d5817c4c354" }, 
	  "name" : "getAll", 
	  "func" : "function getAll() {return db.sample.employee.find();}", 
	  "funcType" : 0 
	}
	```

* 指定返回函数名为 sum 的记录

	```lang-javascript
	> db.listProcedures({name:"sum"})
	{ "_id" : { "$oid" : "52480389f5ce8d5817c4c353" }, 
	  "name" : "sum", 
	  "func" : "function sum(x, y) {return x + y;}", 
	  "funcType" : 0 
	}
	```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[STOREPROCEDURES]:manual/Manual/Catalog_Table/STOREPROCEDURES.md
