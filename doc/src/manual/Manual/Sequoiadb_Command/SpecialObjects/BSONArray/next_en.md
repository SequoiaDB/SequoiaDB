##NAME##

next - Get the next element.

##SYNOPSIS##

***BSONArray.next()***

##CATEGORY##

BSONArray

##DESCRIPTION##

Get the next element.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns the next element.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONArray.( For more detail about oma, please reference to [Oma](reference/Sequoiadb_command/Oma/Oma.md) )

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> var bsonArray = oma.listNodes()
```

* Get the next element.

```lang-javascript
> bsonArray.next()
{
  "svcname": "20000",
  "type": "sequoiadb",
  "role": "data",
  "pid": 862,
  "groupid": 0,
  "nodeid": 0,
  "primary": 0,
  "isalone": 0,
  "groupname": "",
  "starttime": "2019-07-26-14.12.27",
  "dbpath": "/opt/trunk/database/20000/"
}
```
