
##NAME##

traceStatus - Show the tracking status of the current program.

##SYNOPSIS##

***db.traceStatus()***

##CATEGORY##

Sdb

##DESCRIPTION##

Show the tracking status of the current program.

##PARAMETERS##

None

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* View the tracking status of the current program. 

```lang-javascript
> db.traceStatus()
{
  "TraceStarted": true,
  "Wrapped": false,
  "Size": 524288,
  "Mask": 
  [
  "auth",
  "bps",
  "cat",
  "cls",
  "dps",
  "mig",
  "msg",
  "net",
  "oss",
  "pd",
  "rtn",
  "sql",
  "tools",
  "employee",
  "client",
  "coord",
  "dms",
  "ixm",
  "mon",
  "mth",
  "opt",
  "pmd",
  "rest",
  "spt",
  "util",
  "aggr",
  "spd",
  "qgm"
  ],
  "BreakPoint": [],
  "Threads": [],
}
```
