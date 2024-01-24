
##NAME##

getSlave - Get the slave node of current replica group.

##SYNOPSIS##

**rg.getSlave([positions])**

##CATEGORY##

Replica Group

##DESCRIPTION##

Get the slave node of current replica group.

##PARAMETERS##

* `positions` ( *Int32*， *Optional* )

	positions of nodes. Should be [1,7]. The amount of valid positions can not more than 7. 

##RETURN VALUE##

On success, return an object of SdbNode.

On error, exception will be thrown.

**Note:**

1. when only have a node in current replica group, return this node anyway.
2. when have several nodes in current replica group, while no position is specified, return a random slave node.
3. when have several nodes in current replica group, while specified positions, and 
the positions contain slave node, return a random slave node first.


##ERRORS##

the exceptions of `getSlave()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Invalid arguments. | Check the input positions. |
| -10 | SDB_SYS | Network error. | Check the meta data of node in catalog is ok or not. |
| -154 | SDB_CLS_GRP_NOT_EXIST | The replica group does not exist. | Check the current group exist or not. |
| -158 | SDB_CLS_EMPTY_GROUP | The current replica group is empty. | Check the current group has nodes or not. |

When error happen, use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more detail.

##HISTORY##

* add the positions arguments since v2.10
* since v1.0

##EXAMPLES##

1. Get a slave node in group1.

	```lang-javascript
	> var rg = db.getRG("group1")
	> rg.getSlave()
	hostname1:42000
	```

2. Get the slave node in group1 with the specified positions.

	```lang-javascript
	> var rg = db.getRG("group1")
	> rg.getSlave(1,2)
	hostname1:40000
	```
