
##NAME##

md5 - Get the md5 value of the file.

##SYNOPSIS##

***File.md5( \<filepath\> )***

##CATEGORY##

File

##DESCRIPTION##

Get the md5 value of the file.

##PARAMETERS##

| Name     | Type   | Default | Description | Required or not |
| -------- | ------ | ------- | ----------- | --------------- |
| filepath | string | ---     | file path   | yes             |

##RETURN VALUE##

On success, return file's md5 value.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Get the md5 value of the file.

```lang-javascript
> File.md5( "/opt/sequoiadb/file.txt" )
f8fef4e0f30176c126d85cadca298a7c
```