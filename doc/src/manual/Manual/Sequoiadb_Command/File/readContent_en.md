
##NAME##

readContent - Read file's content into a FileContent object.

##SYNOPSIS##

***File.readContent( \[size\] )***

##CATEGORY##

File

##DESCRIPTION##

Read file's content into a FileContent object.

##PARAMETERS##

| Name   | Type     | Description                              | Required or not |
| ------ | -------- | ---------------------------------------  | --------------- |
| size   | int      | the number of bytes requested to be read and the entire contents of the  current file cursor will be readed by default | not             |

##RETURN VALUE##

On success, the number of bytes read is returned, and the file position is advanced by this number.

On error, exception will be thrown.

##ERRORS##

when exception happens, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open an existing or new file and get a file descriptor

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.dump" )
```

* Read the contents of the file into the fileContent object

```lang-javascript
> var content = file.readContent()
> content instanceof FileContent   // Verify that content is a 'FileContent' object
true
```