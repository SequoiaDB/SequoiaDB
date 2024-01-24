##NAME##

toJson - Convert BSONObj to JSON format.

##SYNOPSIS##

***BSONObj.toJson()***

##CATEGORY##

BSONObj

##DESCRIPTION##

Convert BSONObj to JSON format.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, returns JSON format data of BSONObj.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a BSONObj.

```lang-javascript
> var db = new Sdb( "localhost", 11810 )
> var bsonObj = db.foo.bar.find().current()
```

* Convert BSONObj to JSON format.

```lang-javascript
> bsonObj.toJson() 
{ "_id": { "$oid": "5d240ab1117b8a87cbfd10eb" }, "age": 17, "name": "tom" }
```
