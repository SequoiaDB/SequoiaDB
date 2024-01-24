
##名称##

setLastErrMsg - 设置前一次操作的详细错误信息。

##语法##

**setLastErrMsg(\<msg\>)**

##类别##

Global

##描述##

设置前一次操作的详细错误信息。可通过该接口为前一次操作设置更详细的错误信息。

##参数##

* `msg` ( *String*， *必填* )

	错误信息。

##返回值##

无。

##错误##

无。

##版本##

v2.6及以上版本。

##示例##

1. 修改前一次操作的详细错误信息。

  	```lang-javascript
  	> db = new Sdb()
  	(nofile):0 uncaught exception: -15
  	> var err = getLastErrMsg()
	> err = Date() + ": " + err
	Wed May 24 2017 12:44:44 GMT+0800 (CST): Network error
	> setLastErrMsg( err ) ;
  	```