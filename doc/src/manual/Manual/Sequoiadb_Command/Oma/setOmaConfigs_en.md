
##NAME##

setOmaConfigs - Set the configuration information to the configuration file of sdbcm.

##SYNOPSIS##

**oma.setOmaConfigs(\<config\>,[confFile])**

##CATEGORY##

Oma

##DESCRIPTION##

Set the configuration information to the configuration file of sdbcm.

##PARAMETERS##

* `config` ( *Object*， *Required* )

	The configuration information for sdbcm.

* `confFile` ( *String*， *Optional* )

	The configuration file of sdbcm, default to be the file of [getOmaConfigFile()](manual/Manual/Sequoiadb_Command/Oma/getOmaConfigFile.md), when `confFile` is not specified.

##RETURN VALUE##

On success, nothing returns.

On error, exception will be thrown.

##ERRORS##

the exceptions of `setOmaConfigs()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -4 | SDB_FNE | File not exist. | Check the configuration file of sdbcm exists or not. |

##HISTORY##

Since v2.0.

##EXAMPLES##

1. Reset the dialog level from "ERROR" to "DEBUG", and then make it works.

	```lang-javascript
	> var ret = Oma.getOmaConfigs()
	> var obj = ret.toObj()
	> println(obj["DiagLevel"])
	3
	> obj["DiagLevel"] = 5
	> Oma.setOmaConfigs(obj)
	> var oma = new Oma()
 	> oma.reloadConfigs()
	```