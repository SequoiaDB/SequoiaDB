##NAME##

attachNode - Add a node that has been created but does not belong to any group to the current group.

##SYNOPSIS##

**rg.attachNode( \<host\>, \<service\>, \<options\> )**

##CATEGORY##

Replica Group

##DESCRIPTION##

Add a node that has been created but does not belong to any group to the current group. Can be used with [rg.detachNode()](manual/Manual/Sequoiadb_command/SdbReplicaGroup/detachNode_en.md).

##PARAMETERS##

* `host` ( *String*， *Required* )

	Hostname or IP address of node. 

* `service` ( *String*， *Required* )

	Service name or port of node. 

* `options` ( *Object*, *Required* )

    Can be the following options:

    1. `KeepData` ( *Bool* ): Whether to keep the original data of the new node. This option has no default value. User should specify its value explicitly.

**Note:**

1. The field `KeepData` in the `options` must be specified explicitly. For it will determine if the data of the new node still be kept or not. So, be careful.
2. It's better to set `KeepData` to be `false` if the new node does not originally belong to the current group.
3. Nodes in the node configuration file where roles are specified as `catalog` can only be added to the catalog group; nodes whose roles are specified as `data` can only be added to data groups.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

the exceptions of `attachNode()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | Network error. | 1. Check the state of sdbcm. 2. Check whether hostname or service name is ok or not. |
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist. | Check whether the note exists or not. |
| -157 | SDB_CM_CONFIG_CONFLICTS | Node may have been in another group. | Check if the node has joined to the current or other replication group. If it belongs to any replication group, this operation will not be supported. |

When error happen, use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more detail.

##HISTORY##

* since v1.12

##EXAMPLES##

1. Detach node from group1 and then add it to group2.

	```lang-javascript
	> var rg1 = db.getRG("group1")
	> rg1.detachNode('hostname1', '11830', { KeepData: true })
	> var rg2 = db.getRG("group2")
	> rg2.attachNode('hostname1', '11830', { KeepData: false })
	```