
##NAME##

removeCoord - Remove the specified coord node in target host of sdbcm.

##SYNOPSIS##

**oma.removeCoord(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Remove the specified coord node in target host of sdbcm.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `removeCoord()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -3 | SDB_PERM | Permission error. | Check if the node path is correct and the path permissions are correct. |
| -15 | SDB_NETWORK | Network error. | 1.Check if the sdbcm status is normal, if the status is abnormal, you can try restart.  2.Check network conditions. |
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist. | Check if the node exists. |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Delete the coord node with port number 11810 on the target host sdbserver1.

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.removeCoord( 11810 )
 	```
