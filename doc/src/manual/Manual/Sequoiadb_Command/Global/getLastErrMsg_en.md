
##NAME##

getLastErrMsg - Get the error message of last operation.

##SYNOPSIS##

**getLastErrMsg()**

##CATEGORY##

Global

##DESCRIPTION##

When error happen, we can use this API to get the error message.

##PARAMETERS##

NULL.

##RETURN VALUE##

An error message string or an empty string for no error happen.

##ERRORS##

NULL.

##HISTORY##

Since v2.6.

##EXAMPLES##

1. Get the error message of the last operation.

	```lang-javascript
    > db = new Sdb()
    (nofile):0 uncaught exception: -15
    > getLastErrMsg()
    Network error
	```
