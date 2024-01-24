
##NAME##

chown - Set the user and/or group ownership of each given file.

##SYNOPSIS##

***File.chown( \<filepath\>, \<options\>, \[recursive\] )***

##CATEGORY##

File

##DESCRIPTION##

Set the user and/or group ownership of each given file.
##PARAMETERS##

| Name      | Type     | Defaults | Description                  | Required or not |
| --------- | -------- | -------- | ---------------------------- | --------------- |
| filepath  | string   | ---      | source file path             | yes             |
| options   | JSON     | ---      | usename or groupname         | yes             |
| recursive | boolean  | false    | whether recursive processing | not             |

The detailed description of 'options' parameter is as follows:

| Name      | Type   | Description | Required or not |
| --------- | ------ | ----------- | --------------- |
| username  | string | username    | not             |
| groupname | string | groupname   | not             |


>Note:

>Two parameters, username and groupname, must specify one of them.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Changes the user and group ownership of each given file.

```lang-javascript
> File.chown( "/opt/sequoiadb/file.txt", { "username": "sequoiadb", "groupname": "SequoiadDB" }, false )
```