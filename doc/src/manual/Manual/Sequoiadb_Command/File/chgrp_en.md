
##NAME##

chgrp - Set the user group of the file.

##SYNOPSIS##

***File.chgrp( \<filepath\>, \<groupname\>, \[recursive\] )***

##CATEGORY##

File

##DESCRIPTION##

Set the user group of the file.

##PARAMETERS##

| Name      | Type     | Default | Description                  | Required or not |
| --------- | -------- | ------- | ---------------------------- | --------------- |
| filepath  | string   | ---     | source file path             | yes             |
| groupname | string   | ---     | groupname                    | yes             |
| recursive | boolean  | false   | whether recursive processing | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Set the user group of the file to the "sequoiadb".

```lang-javascript
> File.chgrp( "/opt/sequoiadb/file.txt", "sequoiadb", false )
```