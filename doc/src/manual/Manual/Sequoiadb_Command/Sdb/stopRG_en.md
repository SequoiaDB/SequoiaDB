##NAME##

stopRG - stop the replication group

##SYNOPSIS##

**db.stopRG(\<name1\>, [name2], ...)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to stop the specified replication group. After stopping, users cannot perform related operations such as creating nodes. This method is equivalent to [rg.stop()][stop].

##PARAMETERS##


| Name | Type    | Description 			| Required or not 	 |
| ------ | ------ 	| ------ 		| ------	 |
| name1, name2... 	 | string 	| Replication group name. 	| required		 |

> **Note:**
>
> - If the specified replication group does not exist, an exception will be thrown.
> - If no replication group is specified, this operation is a nop.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

The command to stop the replication group is as follow:

```lang-javascript
> db.stopRG("group1")
> db.stopRG("group2", "group3", "group4")
```


[^_^]:
   links
[stop]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stop.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md