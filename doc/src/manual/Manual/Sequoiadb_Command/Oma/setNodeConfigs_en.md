
##NAME##

setNodeConfigs - Use the new configuration information to overwrite the contents in the configuration file of the specified SequoiaDB node.

##SYNOPSIS##

**oma.setNodeConfigs(\<svcname\>,\<config\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Use the new configuration information to overwrite the contents in the configuration file of the specified SequoiaDB node.

**Note:**

* After overwrite, the original content in the configuratino file will be lost.

* Use reloadConf() reload the configuration file.

* Use updateConf() and deleteConf() modify configuration online.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

* `config` ( *Object*， *Required* )

	Node configuration information, specific reference database configuration.

##RETURN VALUE##

On success, no return value. 

On error, exception will be thrown.

##ERRORS##

the exceptions of `setNodeConfigs()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error. | Check if the parameters are correct. |
| -259 | SDB_OUT_OF_BOUND | No node port number or configuration information entered | Enter the node port number or configuration information |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.
##HISTORY##

since v2.0

##EXAMPLES##

1. Overwrite the configuration of the node with port number 11810 with the new configuration.

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.setNodeConfigs( 11810, { svcname: "11810", dbpath: "/home/users/sequoiadb/trunk/11810", diaglevel: 3, clustername: "xxx", businessname: "yyy", role: "data", catalogaddr: "ubuntu1:11823, ubuntu2:11823" } )
	```