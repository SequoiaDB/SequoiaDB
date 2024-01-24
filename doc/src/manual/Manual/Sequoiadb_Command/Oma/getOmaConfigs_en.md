
##NAME##

getOmaConfigs - Get the configuration information of sdbcm.

##SYNOPSIS##

**oma.getOmaConfigs()**

##CATEGORY##

Oma

##DESCRIPTION##

Get the config information of sdbcm.

##PARAMETERS##

* `confFile` ( *String*， *Optional* )

	The configuration file of sdbcm, default to be the file of [getOmaConfigFile()](manual/Manual/Sequoiadb_command/Oma/getOmaConfigFile.md), when `confFile` is not specified.

##RETURN VALUE##

On success, return an object of BSONObj, which contains the configuration information of sdbcm.

On error, exception will be thrown.

##ERRORS##

the exceptions of `getOmaConfigs()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -4 | SDB_FNE | File not exist. | Check the configuration file of sdbcm exists or not. |

##HISTORY##

Since v1.0.

##EXAMPLES##

1. print the configuration information of sdbcm.

	```lang-javascript
	> Oma.getOmaConfigs()
	{
  		"AutoStart": true,
		"DiagLevel": 3,
  		"EnableWatch": "TRUE",
  		"IsGeneral": "TRUE",
  		"OMAddress": "rhel64-test8:11785",
  		"RestartCount": 5,
  		"RestartInterval": 0,
  		"defaultPort": "11790",
  		"rhel64-test8_Port": "11790"
	}
 	```

2. Get the information from configuration file of sdbcm.

	```lang-javascript
	> var ret = Oma.getOmaConfigs()
	> var obj = ret.toObj()
	> println(obj["DiagLevel"])
	3
	```