
##NAME##

readLine - Read one entire line from a file.

##SYNOPSIS##

***File.readLine()***

##CATEGORY##

File

##DESCRIPTION##

Read one entire line from a file.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, the line readed is returned, and the file position is advanced by this line.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open a file and get a file descriptor;

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.txt" )
> file.read()
0:sequoiadb is wonderful.
1:wonderful sequoiadb.
```

* Read file.

```lang-javascript
> file.seek(0)
> file.readLine()
0:sequoiadb is wonderful. 
> file.readLine()
1:wonderful sequoiadb.
```