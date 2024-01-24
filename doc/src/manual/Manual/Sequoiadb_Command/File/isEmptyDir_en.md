
##NAME##

isEmptyDir - Determine if the directory is empty.

##SYNOPSIS##

***File.isEmptyDir( \<dirName\> )***

##CATEGORY##

File

##DESCRIPTION##

Determine if the directory is empty.

##PARAMETERS##

| Name    | Type     | Default | Description        | Required or not |
| ------- | -------- | ------- | ------------------ | --------------- |
| dirName | string   | ---     | directory pathname | yes             |

##RETURN VALUE##

Return true if the specified directory is empty, or return false.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Determine if the directory is empty.

```lang-javascript
> File.isEmptyDir( "/opt/sequoiadb" )
false
```