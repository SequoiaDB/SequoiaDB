
##名称##

getLastErrMsg - 获取前一次操作的详细错误信息。

##语法##

**getLastErrMsg()**

##类别##

Global

##描述##

获取前一次操作的详细错误信息。

##参数##

无。

##返回值##

若前一次操作发生错误，该函数返回错误详细信息；否则，无内容返回。

##错误##

无。

##版本##

v2.6及以上版本。

##示例##

1. 获取前一次操作的详细错误信息。

  	```lang-javascript
  	> db = new Sdb()
  	(nofile):0 uncaught exception: -15
  	> getLastErrMsg()
  	Network error
  	```