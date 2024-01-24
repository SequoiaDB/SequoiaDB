##NAME##

pos - Get the element of current subscript.

##SYNOPSIS##

***BSONArray.pos()***

##CATEGORY##

BSONArray

##DESCRIPTION##

Get the element of current subscript.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns the current elememt.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONArray.( For more detail about oma, please reference to [Oma](reference/Sequoiadb_command/Oma/Oma.md) )

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> var bsonArray = oma.listNodes()
```

* Get the element of current subscript.

```lang-javascript
> bsonArray.pos()
{
  "svcname": "11820",
  "type": "sequoiadb",
  "role": "standalone",
  "pid": 17936,
  "groupid": 0,
  "nodeid": 0,
  "primary": 1,
  "isalone": 0,
  "groupname": "",
  "starttime": "2019-07-26-16.43.01",
  "dbpath": "/opt/trunk/database/standalone/11820/"
}
```
