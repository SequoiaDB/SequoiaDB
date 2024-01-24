
##NAME##

getAOmaSvcName - Get the service name of sdbcm in target host.

##SYNOPSIS##

**oma.getAOmaSvcName(\<hostname\>,[confFile])**

##CATEGORY##

Oma

##DESCRIPTION##

Get the service name of sdbcm in target host.

##DESCRIPTION##

* `hostname` ( *String*， *Required* )

	The hostname of the target host.

* `configFile` ( *String*， *Optional* )

	The configuration file path, If not filled will user the default configuration file: /opt/sequoiadb/conf/sdbcm.conf.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `getAOmaSvcName()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -4 | SDB_FNE | File does not exist. | Check the configuration file path is it right or not. |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Use getAOmaSvcName() to get the service name of sdbcm from target host sdbserver1.

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.delAOmaSvcName( "sdbserver1")
    ```