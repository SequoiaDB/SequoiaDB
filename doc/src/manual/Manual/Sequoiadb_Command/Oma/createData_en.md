
##NAME##

createData - Create a standalone node in target host of sdbcm.

##SYNOPSIS##

**oma.createData(\<svcname\>,\<dbpath\>,[config])**

##CATEGORY##

Oma

##DESCRIPTION##

Create a standalone node in target host of sdbcm.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

* `dbpath` ( *String*， *Required* )

	The node data directory.

* `config` ( *Object*， *Optional* )

	Node configuration information.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `createCoord()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -3 | SDB_PERM | Permission error. | Check if the node path is correct and the path permissions are correct. |
| -15 | SDB_NETWORK | Network error. | 1.Check if the sdbcm status is normal, if the status is abnormal, you can try restart.  2.Check network conditions. |
| -145 | SDBCM_NODE_EXISTED | Node already exist. | Check if the node already exists. |
| -157 | SDB_CM_CONFIG_CONFLICTS | Node configuration conflict. | Check if the port and data directory are used. |

When error happen, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Create a standalone node with port number 11820 and specify a log file size of 64 MB.

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.createData( 11820, "/opt/sequoiadb/standlone/11820", { logfilesz: 64 } )
    ```