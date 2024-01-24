##NAME##

removeRG - remove the replication group 

##SYNOPSIS##

**db.removeRG(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to remove the specified replication group in the database, and the replication group must exist.

##PARAMETERS##

| Name 		| Type   	| Description 			| Required or not 	|
| ------ 		| ------ 	| ------ 		| ------ 	|
| name 	 | string	| Replication group name. In the same database object, the replication group name is unique. 	| required 		|

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Remove the replication group named "group".

```lang-javascript
> db.removeRG("group")
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md