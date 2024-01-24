
##NAME##

close - Close the opened file.

##SYNOPSIS##

***File.close()***

##CATEGORY##

File

##DESCRIPTION##

Close the opened file.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open a file and get a file descriptor;

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.txt" )
```

* Close the opened file.

```lang-javascript
> file.close()
```