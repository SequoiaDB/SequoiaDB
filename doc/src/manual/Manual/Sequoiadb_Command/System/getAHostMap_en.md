
##NAME##

getAHostMap - Acquire a hostname to ip address mapping in the host file

##SYNOPSIS##

***System.getAHostMap( \<hostname\> )***

##CATEGORY##

System

##DESCRIPTION##

Acquire a hostname to ip address mapping in the host file

##PARAMETERS##

| Name      | Type     | Default | Description   | Required or not |
| ------- | -------- | ------------ | ---------- | -------- |
| hostname     | string   | ---     | hostname       | yes       |

##RETURN VALUE##

On success, return corresponding ip address.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire a hostname to ip address mapping in the host file

```lang-javascript
> System.getAHostMap( "localhost" )
127.0.0.1
```