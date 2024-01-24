
##NAME##

writeContent - Write a fileContent object to file.

##SYNOPSIS##

***File.writeContent( \<fileContent\> )***

##CATEGORY##

File

##DESCRIPTION##

Write to a fileContent object to file.

##PARAMETERS##

| Name        | Type               | Default | Description                 | Required or not |
| ----------- | ------------------ | ------- | --------------------------- | --------------- |
| fileContent | fileContent object | ---     | what is written to the file | yes             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open a binary file and get a file descriptor;

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.dump" )
```

* Read the contents of the file into the fileContent object;

```lang-javascript
> var content = file.readContent()
> content instanceof FileContent
true
```

* Write file.

```lang-javascript
> var file2 = new File( "/opt/sequoiadb/file2.dump" )
> file2.writeContent( content )
```