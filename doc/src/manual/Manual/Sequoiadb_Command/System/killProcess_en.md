
##NAME##

killProcess - kill the process

##SYNOPSIS##

***System.killProcess( \<options\> )***

##CATEGORY##

System

##DESCRIPTION##

kill the process

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| options | JSON   | ---    |  process information  | yes   |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | -------------- |
| pid    | int |  yes   | { pid: 31831 }   | process id             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

kill the process

```lang-javascript
> System.killProcess( { pid: 31831 } )
```