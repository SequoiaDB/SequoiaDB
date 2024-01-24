
##NAME##

setUmask - Set file mode creation mask.

##SYNOPSIS##

***File.setUmask( \<umask\> )***

##CATEGORY##

File

##DESCRIPTION##

Set file mode creation mask.

##PARAMETERS##

| Name  | Type | Default | Description                 | Required or not |
| ----- | ---- | ------- | --------------------------- | --------------- |
| umask | int  | ---     | permission mask of new file | yes             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happens, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Set file mode creation mask.

```lang-javascript
> File.setUmask( 0664 )
```