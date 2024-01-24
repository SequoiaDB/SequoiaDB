
##NAME##

snapshotNetcardInfo - Acquire detailed network card information

##SYNOPSIS##

***System.snapshotNetcardInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire detailed network card information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return detailed network card information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire detailed network card information

```lang-javascript
> System.snapshotNetcardInfo()
{
  "CalendarTime": 1559722067,
  "Netcards": [
    {
      "Name": "lo",
      "RXBytes": 108885345140,
      "RXPackets": 97058303,
      "RXErrors": 0,
      "RXDrops": 0,
      "TXBytes": 108885345140,
      "TXPackets": 97058303,
      "TXErrors": 0,
      "TXDrops": 0
    },
    {
      "Name": "ens160",
      "RXBytes": 8267964446,
      "RXPackets": 6629177,
      "RXErrors": 0,
      "RXDrops": 141152,
      "TXBytes": 1864089945,
      "TXPackets": 2306206,
      "TXErrors": 0,
      "TXDrops": 0
    }
  ]
}
```