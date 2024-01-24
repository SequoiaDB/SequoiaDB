##NAME##

toObj - Convert BSONObj to a JSON object.

##SYNOPSIS##

***BSONObj.toObj()***

##CATEGORY##

BSONObj

##DESCRIPTION##

Convert BSONObj to a JSON object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns Json object of BSONObj.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONObj.

```lang-javascript
> var db = new Sdb( "localhost", 11810 )
> var bsonObj = db.foo.bar.find().current()
```

* Convert BSONObj to a JSON object.

```lang-javascript
> var obj = bsonObj.toObj()
> obj.age
17
> obj.name
tom 
```
