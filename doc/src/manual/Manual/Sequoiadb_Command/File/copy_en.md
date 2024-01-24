
##NAME##

copy - Copy file.

##SYNOPSIS##

***File.copy( \<src\>, \<dst\>, \[replace\], \[mode\] )***

##CATEGORY##

File

##DESCRIPTION##

Copy file.

##PARAMETERS##

| Name    | Type     | Default                   | Description                     | Required or not |
| ------- | -------- | ------------------------- | ------------------------------- | --- |
| src     | string   | ---                       | source file path                | yes |
| des     | string   | ---                       | destination file path           | yes |
| replace | boolean  | true                      | whether replace the source file | not |
| mode    | int      | source file's permissions | set file permissions            | not |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Copy file.

```lang-javascript
> File.copy( "/opt/sequoiadb/srcFile.txt", "/opt/sequoiadb/desFile.txt", true, 0664 )
```