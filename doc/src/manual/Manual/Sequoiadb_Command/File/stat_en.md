
##NAME##

stat - Display file or file system status.

##SYNOPSIS##

***File.stat( \<filepath\> )***

##CATEGORY##

File.

##DESCRIPTION##

Display file or file system status

##PARAMETERS##

| Name     | Type   | Default | Description | Required or not |
| -------- | ------ | ------- | ----------- | --------------- |
| filepath | string | ---     | file path   | yes             |


##RETURN VALUE##

On success, return file status information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Display file or file system status.

```lang-javascript
> File.stat( "/opt/sequoiadb/file.txt" )
{
  "name": "/opt/sequoiadb/file.txt",
  "size": "0",
  "mode": "rw-r--r--",
  "user": "root",
  "group": "root",
  "accessTime": "2019-06-14 14:04:58.883619463 +0800",
  "modifyTime": "2019-06-14 14:04:58.883619463 +0800",
  "changeTime": "2019-06-14 14:04:58.883619463 +0800",

  "type": "regular file"
}
```