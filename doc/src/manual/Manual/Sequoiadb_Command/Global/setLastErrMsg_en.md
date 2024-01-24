
##NAME##

setLastErrMsg - Set the error message of last operation.

##SYNOPSIS##

**setLastErrMsg(\<msg\>)**

##CATEGORY##

Global

##DESCRIPTION##

When error happen, we can use this API to set the error message.

##PARAMETERS##

* `msg` ( *String*， *Required* )

	The error massage to set.

##RETURN VALUE##

NULL.

##ERRORS##

NULL.

##HISTORY##

Since v2.6.

##EXAMPLES##

1. Reset the error message of last operation.

  	```lang-javascript
  	> db = new Sdb()
  	(nofile):0 uncaught exception: -15
  	> var err = getLastErrMsg()
	> err = Date() + ": " + err
	Wed May 24 2017 12:44:44 GMT+0800 (CST): Network error
	> setLastErrMsg( err ) ;
  	```
