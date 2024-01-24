##NAME##

more - Determine if the next element isn't null.

##SYNOPSIS##

***BSONArray.more()***

##CATEGORY##

BSONArray

##DESCRIPTION##

Determine if the next element isn't null.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns true if the next elememt isn't null, or return false.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONArray.( For more detail about oma, please reference to [Oma](reference/Sequoiadb_command/Oma/Oma.md) )

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> var bsonArray = oma.listNodes()
```

* Determine if the next element isn't null

```lang-javascript
> bsonArray.more()
true
```
