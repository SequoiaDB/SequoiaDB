
##NAME##

delAHostMap - Delete a hostname to ip address mapping in the host file

##SYNOPSIS##

***System.delAHostMap( \<hostname\> )***

##CATEGORY##

System

##DESCRIPTION##

Delete a hostname to ip address mapping in the host file

##PARAMETERS##

| Name      | Type     | Default | Description   | Required or not |
| ------- | -------- | ------------ | ---------- | -------- |
| hostname     | string   | ---     | hostname       | yes       |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Delete a hostname to ip address mapping in the host file

```lang-javascript
> System.delAHostMap( "hostname" )
```