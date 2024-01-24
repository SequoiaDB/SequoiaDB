
##NAME##

addAOmaSvcName - Specify the service name of sdbcm in target host.

##SYNOPSIS##

**Oma.addAOmaSvcName(\<hostname\>,\<svcname\>,[isReplace],[confFile])**

##CATEGORY##

Oma

##DESCRIPTION##

Specify the service name of sdbcm in target host.

##DESCRIPTION##

* `hostname` ( *String*， *Required* )

	The hostname of the target host.

* `svcname` ( *Int | String*， *Required* )

	The port of the target sdbcm, and the port cannot be occupied.

* `isReplace` ( *Bool*， *Optional* )

	Whether to replace the configuration file.

* `configFile` ( *String*， *Optional* )

	The configuration file path, use the default configuration file if not filled.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `addAOmaSvcName()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -4 | SDB_FNE | File does not exist. | Check the configuration file path is it right or not. |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more detail.

##HISTORY##

since v2.0

##EXAMPLES##

1. Specify the service name 11780 of sdbcm in target host sdbserver1.

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.addAOmaSvcName( "sdbserver1", 11780, false)
	```

2. The configuration file "sdbcm.conf" will add the following line:

	```lang-javascript
 	sdbserver1_Port=11780
	```