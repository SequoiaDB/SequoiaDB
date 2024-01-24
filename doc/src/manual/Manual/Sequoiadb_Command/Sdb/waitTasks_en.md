##NAME##

waitTasks - synchronously wait for the specified task to end or cancel

##SYNOPSIS##

**db.waitTasks(\<id1\>, [id2], ...)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to synchronously wait for the specified task to end or cancel.

##PARAMETERS##

| Name 	| Type 	   | Description 		| Required or not 	|
| ------ 	| ------ 	| ------ 	| ------ 	|
| id1, id2… | number 		| Tak ID 	| required	 	|

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Synchronously wait for the task segmentation to complete.

```lang-javascript
> var taskid1 = db.test.test.splitAsync("db1", "db2", 50);
> var taskid2 = db.my.my.splitAsync("db3", "db4", 50) ;
> db.waitTasks(taskid1, taskid2)
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md