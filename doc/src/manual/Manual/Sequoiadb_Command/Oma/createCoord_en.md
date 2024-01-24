##NAME##

createCoord - create a temporary coordination node

##SYNOPSIS##

**oma.createCoord(\<svcname\>, \<dbpath\>, [config])**

##CATEGORY##

Oma

##DESCRIPTION##

This function is used to create a temporary coordination node in the machine where the resource management node (sdbcm) is located, for the initial creation of the SequoiaDB cluster. The coordination node created by this function will not be registered in the catalog node, that is the node cannot be managed by the cluster. If users want the coordination node to be managed by the cluster, refer to [createNode()][createNode].

##PARAMETERS##

- svcname ( *string/number, required* )

	Node port number

- dbpath ( *string, required* )

	Node's data storage directory

- config ( *object, optional* )

	Node configuration information, such as configuration log size, whether to open transactions and so on. For more details can refer to [database configuration][configuration].

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createCoord()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -3     | SDB_PERM | Permission error| Check whether the node path is correct and whether the path permissions are correct |
| -15    | SDB_NETWORK | Network Error| Check whether the network status or sdbcm status is normal, if the status is abnormal, users can try to restart|
| -145   | SDBCM_NODE_EXISTED | Node already exists| Check whether the node exists |
| -157   | SDB_CM_CONFIG_CONFLICTS | Node configuration conflict | Check whether the port number and data directory have been used |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Connect to the local cluster management service process sdbcm and create a temporary coordination node with port number 18800.

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.createCoord(18800, "/opt/sequoiadb/database/coord/18800")
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[createNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md
    