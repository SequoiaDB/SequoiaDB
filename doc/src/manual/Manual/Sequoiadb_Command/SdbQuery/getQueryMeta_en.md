
##NAME##

getQueryMeta - Get query metadata information.

##SYNOPSIS##

***query.getQueryMeta()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get query metadata information.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return query metadata information.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Get query metadata information.

```lang-javascript
> db.sample.employee.find().getQueryMeta()
{
  "HostName": "ubuntu",
  "ServiceName": "42000",
  "NodeID": [
    1001,
    1003
  ],
  "ScanType": "tbscan",
  "Datablocks": [
    9
  ]
} 
```