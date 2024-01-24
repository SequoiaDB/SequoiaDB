##NAME##

getMaster - get the primary node of the current replication group

##SYNOPSIS##

**rg.getMaster()**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to get the primary node of the current replication group.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbNode.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Get the primary node of the "group1" replication group. Node-level operations through this node can be performed.

```lang-javascript
> var rg = db.getRG("group1")
> var node = rg.getMaster()
> println(node)
hostname1:11830
> println(node.constructor.name)
SdbNode
> node.help()

   --Instance methods for class "SdbNode":
   connect()                  - Connect the database to the current node.
   getHostName()              - Return the hostname of a node.
   getNodeDetail()            - Return the information of the current node.
   getServiceName()           - Return the server name of a node.
   start()                    - Start the current node.
   stop()                     - Stop the current node.
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md