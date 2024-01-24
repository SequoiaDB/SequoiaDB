
##NAME##

close - Close the Oma object.

##SYNOPSIS##

**oma.close()**

##CATEGORY##

Oma

##DESCRIPTION##

Close the Oma object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, no return value.

On error, no return value or exception.

##ERRORS##

NULL

##HISTORY##

Since v2.0.

##EXAMPLES##

1. Connect to sdbcm in localhost, and then close the connection.

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.close()
	```