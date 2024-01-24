
##NAME##

setLastError - Set the error code of last operation.

##SYNOPSIS##

**setLastError(\<error code\>)**

##CATEGORY##

Global

##DESCRIPTION##

Set the error number of last operation.

##PARAMETERS##

* `error code` ( *Int32*， *Required* )

	[error code](manual/Manual/Sequoiadb_error_code.md)。

##RETURN VALUE##

NULL.

##ERRORS##

NULL.

##HISTORY##

Since v2.6.

##EXAMPLES##

1. Set the error code of the last opetation.

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