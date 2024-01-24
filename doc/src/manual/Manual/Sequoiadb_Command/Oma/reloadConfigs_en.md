
##NAME##

reloadConfigs - Sdbcm reload the configuration information from the configuration file.

##SYNOPSIS##

**oma.reloadConfigs()**

##CATEGORY##

Oma

##DESCRIPTION##

Sdbcm reload the configuration information from the configuration file.

##PARAMETERS##

NULL

##RETURN VALUE##

NULL

##ERRORS##

NULL

##HISTORY##

Since v2.0.

##EXAMPLES##

1. Reload the configuration information of sdbcm on the target host sdbserver1

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.reloadConfigs()
 	```