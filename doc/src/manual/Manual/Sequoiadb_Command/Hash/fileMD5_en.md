
##NAME##

fileMD5 - Get the md5 value of the file.

##SYNOPSIS##

***Hash.fileMD5( \<filepath\> )***

##CATEGORY##

Hash

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
> Hash.fileMD5( "/opt/sequoiadb/file" )
e2c40f3a729dbe802f2295425d6da21c
```