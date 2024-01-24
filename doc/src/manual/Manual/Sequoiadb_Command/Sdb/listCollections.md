##名称##

listCollections - 枚举集合信息

##语法##

**db.listCollections()**

##类别##

Sdb

##描述##

该函数用于枚举数据库中所有的集合信息。

##参数##

无

##返回值##

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看 [$LIST_CL][LIST_CL]

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##
*  数据库中所有的集合信息

	```lang-javascript
	> db.listCollections()
	{
		"Name": "sample.employee"
	}
	```

[^_^]:
     本文使用的所有引用及链接
[LIST_CL]:manual/Manual/SQL_Grammar/Monitoring/LIST_CL.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md