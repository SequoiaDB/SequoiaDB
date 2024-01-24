
##NAME##

seek - Moving to a specified position in a file.

##SYNOPSIS##

***File.seek( \<offset\>, \[where\] )***

##CATEGORY##

File

##DESCRIPTION##

Moving to a specified position in a file.

##PARAMETERS##

| Name   | Type     | Default | Description   | Required or not |
| ------ | -------- | ------- | ------------- | --------------- |
| offset | int      | ---     | cursor offset | yes             |
| where  | char     | b       | mobile mode   | not             | 

The optional values of the 'where' parameter are as follows:

| Optional values | Description |                           
| ------ | ---------------------------------------------------------------- |
|   b    | File offset is offset                                            |
|   c    | File offset is the offset of the current file cursor plus offset |
|   e    | File offset is file size plus offset                             | 

>Note:

>When the "where" parameter is 'e', the "offset" parameter can be negative.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open a file and get a file descriptor.

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.txt" )
> file.read()
0:sequoiadb is wonderful.
1:wonderful sequoiadb.
```

* Moving cursor, and perform an offset operation from the beginning of the file.

```lang-javascript
> file.seek(2)
> file.read()
sequoiadb is wonderful.
1:wonderful sequoiadb.
> file.seek( 2, "b" )
> file.read()
sequoiadb is wonderful.
1:wonderful sequoiadb.
```

* Moving cursor, and perform an offset operation from the current cursor position of the file.

```lang-javascript
> file.seek(2)
> file.seek( 2, "c" )
> file.read()
quoiadb is wonderful.
1:wonderful sequoiadb.
```

* Offset the file cursor to the end of the file.

```lang-javascript
> file.seek(0)
> file.seek( -5, "e" )
> file.read()
adb.
```