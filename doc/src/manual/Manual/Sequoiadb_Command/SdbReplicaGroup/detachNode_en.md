
##NAME##

detachNode - Detach a node from the current group.

##SYNOPSIS##

**rg.detachNode( \<host\>, \<service\>, \<options\> )**

##CATEGORY##

Replica Group

##DESCRIPTION##

Detach a node in the current partition group, but its configuration information will not be deleted. Used with [rg.attachNode()](manual/Manual/Sequoiadb_command/SdbReplicaGroup/attachNode_en.md). Currently it is possible to support separation of nodes from a data group or a catalog group.

##PARAMETERS##

* `host` ( *String*， *Required* )

	Hostname or IP address of node. 

* `service` ( *String*， *Required* )

	Service name or port of node. 

* `options` ( *Object*, *Required* )

    Can be the following options:

    1. `KeepData` ( *Bool* ): Whether to keep the original data of the detached node. This option has no default value. User should specify its value explicitly.

    2. `Enforced` ( *Bool* ): Whether to detach the node forcibly , default to be false.

**Note:**

1. The field `KeepData` in the `options` must be specified explicitly. For it will determine if the data of the detached node still be kept or not. So, be careful.
2. It can not detach the last node in the replica group unless setting `Enforced` to be `true`.
3. The separated nodes will no longer be managed by the cluster. Please join other groups as soon as possible.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

the exceptions of `attachNode()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | Network error. | 1. Check the state of sdbcm. 2. Check whether hostname or service name is ok or not. |
| -155 | SDB_CLS_NODE_NOT_EXIST | Node does not exist. | Check whether the note exists in current group or not. |
| -204 | SDB_CATA_RM_NODE_FORBIDDEN | Unable to remove the last node or primary in a group. | 1. Check if the node is master. 2. Check if the node is the last node of current group no not. 3. You can use { Enforced: true } |

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
	> rg1.detachNode('hostname1', '11830', {KeepData: true})
	> var rg2 = db.getRG("group2")
	> rg2.attachNode('hostname1', '11830', {KeepData: false})
	```