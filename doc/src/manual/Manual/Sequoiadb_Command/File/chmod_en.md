
##NAME##

chmod - Change file mode bits.

##SYNOPSIS##

***File.chmod( \<filepath\>, \<mode\>, \[recursive\] )***

##CATEGORY##

File

##DESCRIPTION##

Change file mode bits

##PARAMETERS##

| Name      | Type     | Default | Description                     | Required or not |
| --------- | -------- | ------- | ------------------------ | --------------- |
| filepath  | string   | ---     | source file path                | yes             |
| mode      | int      | ---     | set file permissions            | yes             |
| recursive | boolean  | false   | whether recursive processing    | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Set file mode bits as 0777.

```lang-javascript
> File.chmod( "/opt/sequoiadb/file.txt", 0777, false )
```