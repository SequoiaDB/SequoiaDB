
##NAME##

addGroup - Add a user group

##SYNOPSIS##

***System.addGroup( \<groups\> )***

##CATEGORY##

System

##DESCRIPTION##

Add a user group

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| groups     | JSON   | ---    |  user group information  | yes   |

The detail description of 'groups' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | ---------------- |
| name    | string |   yes  | { "name": newGroup }     | user group name       |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Add a user group

```lang-javascript
> System.addGroup( { "name": "newGroup" } )
```