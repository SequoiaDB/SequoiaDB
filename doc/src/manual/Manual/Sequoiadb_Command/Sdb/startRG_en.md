##NAME##

startRG - start the replication group

##SYNOPSIS##

**db.startRG(\<name1\>, [name2], ...)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to start the specified replication group. Only after the replication group is started can nodes be created on the replication group. This method is equivalent to [rg.start()][start].

##PARAMETERS##


| Name | Type    | Description 			| Required or not 	 |
| ------ | ------ 	| ------ 		| ------	 |
| name1, name2... 	 | string 	| Replication group name 	| required		 |

> **Note:**
>
> If the specified replication group does not exist, an exception will be thrown.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

The command to start the replication group is as follow:

```lang-javascript
> db.startRG("group1")
> db.startRG("group2", "group3", "group4")
```


[^_^]:
   links
[start]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md