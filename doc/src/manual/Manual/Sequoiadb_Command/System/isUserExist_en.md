
##NAME##

isUserExist - Determine if a user exists

##SYNOPSIS##

***System.isUserExist( \<name\> )***

##CATEGORY##

System

##DESCRIPTION##

Determine if a user exists

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| ------- | -------- | -------- | ------------ | -------- |
| name     | string   | ---    | user name   | yes       |

##RETURN VALUE##

On success, return true if the user exists, otherwise return false.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Determine if a user exists

```lang-javascript
> System.isUserExist( "root" )
true
```