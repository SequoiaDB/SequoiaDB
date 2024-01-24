
##NAME##

startNode - Start a node in target host of sdbcm.

##SYNOPSIS##

**oma.startNode(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Start a node in target host of sdbcm.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `startNode()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist. | Check if the node exists. |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Start the node with port number 11830. 

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.startNode( 11830 )
 	```