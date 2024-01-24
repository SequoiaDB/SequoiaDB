##名称##

traceResume - 重新开启断点跟踪程序

##语法##

**db.traceResume()**

##类别##

Sdb

##描述##

该函数用于重新开启断点跟踪程序。 db.traceOn() 指定断点开启数据库引擎程序跟踪功能，当被跟踪的模块遇到断点被阻塞， db.traceResume() 可以唤醒被跟踪的模块。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 连接数据节点 20000，开启数据库引擎程序跟踪的功能

	```lang-javascript
	> var data = new Sdb( "localhost", 20000 )
	> data.traceOn( 1000, "dms", "_dmsStorageUnit::insertRecord" )
	```

* 连接到协调节点 50000， sample.employee 集合落在数据节点 20000 上，执行插入操作，操作会被阻塞无法完成
  
	```lang-javascript
	> var db = new Sdb( "localhost", 50000 )
	> db.sample.employee.insert( { _id: 1, name: "a" } )
	```

* 重新开启断点跟踪程序，插入操作执行成功，并返回结果

	```lang-javascript
	> data.traceResume()
	```

* 查看当前断点跟踪程序的状态可参考[traceStatus()](manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md)

	```lang-javascript
	> data.traceStatus()
	```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md