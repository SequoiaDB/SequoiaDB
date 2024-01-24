##NAME##

toArray - Convert BSONArray to array format.

##SYNOPSIS##

***BSONArray.toArray()***

##CATEGORY##

BSONArray

##DESCRIPTION##

Convert BSONArray to array format.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns array format BSONArray.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONArray.( For more detail about oma, please reference to [Oma](reference/Sequoiadb_command/Oma/Oma.md) )

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> var bsonArray = oma.listNodes()
```

* Convert BSONArray to array format.

```lang-javascript
> var array = bsonArray.toArray()
> array[0]
{
  "svcname": "20000",
  "type": "sequoiadb",
  "role": "data",
  "pid": 17929,
  "groupid": 1000,
  "nodeid": 1000,
  "primary": 1,
  "isalone": 0,
  "groupname": "db1",
  "starttime": "2019-07-26-16.43.01",
  "dbpath": "/opt/trunk/database/20000/"
}
```
