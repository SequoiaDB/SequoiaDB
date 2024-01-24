
##NAME##

exist - Determine if the file exists.

##SYNOPSIS##

***File.exist( \<filepath\> )***

##CATEGORY##

File

##DESCRIPTION##

Determine if the file exists.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| filepath | string   | ---     | file path   | yes             |

##RETURN VALUE##

Return true if the file exists.

Return false if the file does exists.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Determine if the file exists.

```lang-javascript
> File.exist( "/opt/sequoiadb/file.txt" )
false
```