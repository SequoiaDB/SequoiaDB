
##NAME##

getNetcardInfo - Acquire network card information

##SYNOPSIS##

***System.getNetcardInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire network card information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return network card information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire network card information

```lang-javascript
> System.getNetcardInfo()
{
  "Netcards": [
    {
      "Name": "lo",
      "Ip": "127.0.0.1"
    },
    {
      "Name": "ens160",
      "Ip": "192.168.20.62"
    }
  ]
}
```