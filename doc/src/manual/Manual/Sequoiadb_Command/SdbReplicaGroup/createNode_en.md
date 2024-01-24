##NAME##

createNode - create a node in the current replication group

##SYNOPSIS##

**rg.createNode(\<host\>, \<service\>, \<dbpath\>, \[config])**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to create a node in the current replication group.

##PARAMETERS##

- host ( *string, required* )

    Hostname

- service ( *number, required* )

    Node port number

- dbpath ( *string, required* )

    The storage path of the node data file.

    >**Note:**
    >
    > The database management user (created when SequoiaDB is installed, the default is sdbadmin) needs to have write permission on the directory specified by this parameter.

- config ( *object, optional* )

    Node configuration information, such as configuration log size, whether to open transactions and so on. Specific configuration can refer to [parameter description][cluster_config].

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbNode.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createNode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -15      |SDB_NETWORK| Network Error     | 1) Check whether the sdbcm status is normal. If the status is abnormal, try to restart sdbcm.<br> 2) Check whether the "hostname/IP" is correct and whether the network can communicate normally. |
| -145     |SDBCM_NODE_EXISTED| Node already exists   | Check whether the node exists. |
| -157     |SDB_CM_CONFIG_CONFLICTS| Node configuration conflict | Check whether the node port is occupied. |
| -3       | SDB_PERM|Permission error     | Check whether the node path and path permissions are correct. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

Create the node "sdbserver1:11830" in the replication group "group1", and specify the size of the synchronization log file to be 64MB.

```lang-javascript
> var rg = db.getRG("group1")
> rg.createNode("sdbserver1", 11830, "/opt/sequoiadb/database/data/11830", {logfilesz: 64})
```

>**Note:**  
>
> Multiple nodes can be created in a replication group, and each node needs to reserve at least five extended ports. Because the system controls five communication interfaces for each node in the background.


[^_^]:
    Links
[cluster_config]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
