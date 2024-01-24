
##名称##

setLastError - 设置前一次操作返回的错误码。

##语法##

**setLastError(\<error code\>)**

##类别##

Global

##描述##

设置前一次操作返回的错误码。

##参数##

* `error code` ( *Int32*， *必填* )

	[错误码](manual/Manual/Sequoiadb_error_code.md)。

##返回值##

无。

##版本##

v2.6及以上版本。

##示例##

1. 设置前一次操作返回的错误码。

	```lang-javascript
  	> db = new Sdb()
  	(nofile):0 uncaught exception: -15
  	> getLastError()
  	-15
  	> getLastError()
  	-15
  	> setLastError(0)
  	> getLastError()
  	0
  	```
