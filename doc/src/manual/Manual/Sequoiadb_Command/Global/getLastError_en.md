
##NAME##

getLastError - Get the error code of last operation.

##SYNOPSIS##

**getLastError()**

##CATEGORY##

Global

##DESCRIPTION##

Get the error number of last operation.

##PARAMETERS##

NULL.

##RETURN VALUE##

An Int32 [error code](manual/Manual/Sequoiadb_error_code.md).

##ERRORS##

NULL.

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Get the error code of the last opetation.

   	```lang-javascript
   	> db = new Sdb()
   	(nofile):0 uncaught exception: -15
   	> getLastError()
    -15
	```