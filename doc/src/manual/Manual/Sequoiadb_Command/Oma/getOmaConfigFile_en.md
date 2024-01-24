
##NAME##

getOmaConfigFile - Get the configuration file of sdbcm.

##SYNOPSIS##

**oma.getOmaConfigFile()**

##CATEGORY##

Oma

##DESCRIPTION##

Get the config file of sdbcm. We can modify the configuration information in this file to change the behavior of sdbcm.

##PARAMETERS##

NULL

##RETURN VALUE##

The name of sdbcm configuration file. Default to be /opt/sequoiadb/conf/sdbcm.conf.

##ERRORS##

NULL

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Get the configuration file of sdbcm.

	```lang-javascript
	> Oma.getOmaConfigFile()
	/opt/sequoiadb/bin/../conf/sdbcm.conf
 	```