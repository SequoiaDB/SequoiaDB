
##NAME##

traceOff -  Turn off the database engine program tracking and export tracking results to binary files.

##SYNOPSIS##

***db.traceOff()***

##CATEGORY##

Sdb

##DESCRIPTION##

Turn off the database engine program tracking and export tracking results to binary files.

##PARAMETERS##

| Name 		| Type 	| Description 				| Required or not 	|
| ------ 		| ------ 	| ------ 			| ------ 	|
| dumpFile 		| string 	| file'name of the dump; if the path of the specified file is a relative path, you must store the file in the `diagpath` directory of the corresponding node. | not 		|

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Turn off the database engine program tracking and export tracking results to binary   files, /opt/sequoiadb/trace.dump.

```lang-javascript
> db.traceOff("/opt/sequoiadb/trace.dump")
```

* Using [traceFmt()](manual/Manual/Sequoiadb_command/Global/traceFmt.md) to analysis the binary file.

```lang-javascript
> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output" )
```
