##NAME##

toString - Convert BSONArray to string format.

##SYNOPSIS##

***BSONArray.toString()***

##CATEGORY##

BSONArray

##DESCRIPTION##

Convert BSONArray to string format.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns string format BSONArray.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONArray.( For more detail about oma, please reference to [Oma](reference/Sequoiadb_command/Oma/Oma.md) )

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> var bsonArray = oma.listNodes()
```

* Convert BSONArray to string format.

```lang-javascript
> var str = bsonArray.toString()
> typeof(str)
string
```
