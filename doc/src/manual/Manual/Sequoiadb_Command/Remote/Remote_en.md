
##NAME##

Remote - Create a Remote object.

##SYNOPSIS##

***var remote = new Remote( [hostname], [svcname] )***

##CATEGORY##

Remote

##DESCRIPTION##

Create a Remote object.

##PARAMETERS##

| Name     | Type   | Default            | Description  | Required or not |
| -------- | ------ | ------------------ | ------------ | --------------- |
| hostname | string | localhost          | IP address   | not             |
| svcname  | int    | local sdbcm's port | sdbcm's port | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a remote object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```