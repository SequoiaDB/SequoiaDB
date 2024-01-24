
##NAME##

ping - Determine whether the host is reachable

##SYNOPSIS##

***System.ping( \<hostname\> )***

##CATEGORY##

System

##DESCRIPTION##

Determine whether the host is reachable

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| ------- | -------- | ------------ | ------------ | -------- |
| hostname     | string   | ---    | hostname | yes       |

##RETURN VALUE##

On success, return true if the network can reach the specified host, otherwise return false.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Determine whether the host is reachable

```lang-javascript
> System.ping( "hostname" )
{
  "Target": "hostname",
  "Reachable": true
}
```