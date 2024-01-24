
##NAME##

setLastErrObj - Set the error object of last operation.

##SYNOPSIS##

**setLastErrObj(\<obj\>)**

##CATEGORY##

Global

##DESCRIPTION##

##PARAMETERS##

* `obj` ( *Object*， *Required* )

	Object for error message.

	The error object contains the follow fields:

	* errno: (Int32) error code.
	* description: (String) the description of error code.
	* detail: (String) error detail.

##RETURN VALUE##

NULL.

##ERRORS##

NULL.

##HISTORY##

Since v2.6.

##EXAMPLES##

1. Set the error object of last operation.

  	```lang-javascript
  	> db = new Sdb()
  	(nofile):0 uncaught exception: -15
  	> var err = getLastErrObj()
	> var obj = err.toObj()
	> println( obj.toString() )
  	{
    	"errno": -15,
    	"description": "Network error",
    	"detail": ""
  	}
	> obj["detail"] = Date() + ": " + obj["description"]
	> setLastErrObj(obj)
  ```