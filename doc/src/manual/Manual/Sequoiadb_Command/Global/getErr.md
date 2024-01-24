
##名称##

getErr - 获取错误码的描述信息。

##语法##

**getErr(\<error code\>)**

##类别##

Global

##描述##

获取前一次操作返回的错误码。

##参数##

* `error code` ( *Int32*， *Required* )

	[错误码](manual/Manual/Sequoiadb_error_code.md).

##返回值##

[错误码](manual/Manual/Sequoiadb_error_code.md)的描述信息。

##版本##

v1.0及以上版本。

##示例##

1. 获取前一次操作返回的错误码。

	```lang-javascript
  	> getErr(-15)
	Network error
  	```
