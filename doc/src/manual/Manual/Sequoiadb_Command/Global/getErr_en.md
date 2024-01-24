
##NAME##

getErr - Get the description of error code.

##SYNOPSIS##

**getErr(\<error code\>)**

##CATEGORY##

Global

##DESCRIPTION##

Get the description of error code.

##PARAMETERS##

NULL.

##RETURN VALUE##

the description of [error code](manual/Manual/Sequoiadb_error_code.md).

##ERRORS##

NULL.

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Get the error code of the last opetation.

	```lang-javascript
  	> getErr(-15)
	Network error
  	```