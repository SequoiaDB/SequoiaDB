
##NAME##

read - Read file.

##SYNOPSIS##

***File.read( \[size\] )***

##CATEGORY##

File

##DESCRIPTION##

Read file.

##PARAMETERS##

| Name   | Type     | Default | Description | Required or not |
| ------ | -------- | ------- | ----------  | --------------- |
| size   | int      | default to read the entire contents of the current file cursor | the number of bytes requested to be read | not |
##RETURN VALUE##

On success, the number of bytes read is returned, and the file position is advanced by this number.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLSES##

* Open a file and get a file descriptor

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.txt" )
```

* Read the contents of the file

```lang-javascript
> file.read()
SquoiaDB
```