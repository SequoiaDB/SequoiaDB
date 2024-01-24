##NAME##

index - Get the current subscript.

##SYNOPSIS##

***BSONArray.index()***

##CATEGORY##

BSONArray

##DESCRIPTION##

Get the current subscript.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns the current subscript.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONArray.( For more detail about oma, please reference to [Oma](reference/Sequoiadb_command/Oma/Oma.md) )

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> var bsonArray = oma.listNodes()
```

* Get the current subscript.

```lang-javascript
> bsonArray.index()
3
```
