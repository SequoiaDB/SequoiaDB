
##NAME##

isGroupExist - Determine if a user group exists

##SYNOPSIS##

***System.isGroupExist( \<name\> )***

##CATEGORY##

System

##DESCRIPTION##

Determine if a user group exists

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| ------- | -------- | ------------ | ------------ | -------- |
| name     | string   | ---    | user group name   | yes       |

##RETURN VALUE##

On success, return true if the user group exists, otherwise return false.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Determine if a user group exists

```lang-javascript
> System.isGroupExist( "root" )
true
```