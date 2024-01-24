
##NAME##

getHostsMap - Acquire all hostnames to ip address mapping in the host file

##SYNOPSIS##

***System.getHostsMap()***

##CATEGORY##

System

##DESCRIPTION##

Acquire all hostnames to ip address mapping in the host file

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return all hostnames to ip address mapping in the host file.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire all hostnames to ip address mapping in the host file

```lang-javascript
> System.getHostsMap()
{
  "Hosts": [
    {
      "ip": "127.0.0.1",
      "hostname": "localhost"
    },
    ...
  ]
} 
```