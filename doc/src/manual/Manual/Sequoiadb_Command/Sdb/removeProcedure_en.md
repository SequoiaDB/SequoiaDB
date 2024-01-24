##NAME##

removeProcedure - remove the specified stored procedure

##SYNOPSIS##

**db.removeProcedure(\<function name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to remove the specified stored procedure. The stored procedure must exist, otherwise an exception massage will appear.

##PARAMETERS##

| Name 		| Type   	| Description 			| Required or not 	|
| ------ 		| ------ 	| ------ 		| ------ 	|
| function name | string 	| Stored procedure name 		| required      |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

| Error Code 		| Error Type | Description 	| Solution										|
| ------ 		| ------ 		| ------										| ------ |
| -233			| SDB_FMP_FUNC_NOT_EXIST | Stored procedure does not exist. | Use the command [listProcedures()][listProcedures] to confirm whether the specified stored procedure exists.	|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Remove the stored procedure named "sum".

```lang-javascript
> db.removeProcedure("sum")
```


[^_^]:
   links
[listProcedures]:manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md