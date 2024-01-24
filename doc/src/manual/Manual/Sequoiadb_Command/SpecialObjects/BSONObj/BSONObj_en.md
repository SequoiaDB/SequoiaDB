##NAME##

BSONObj - Create a new BSONObj object.

##SYNOPSIS##

***BSONObj(\<json\>) / new BSONObj(\<json\>)***

##CATEGORY##

BSONObj

##DESCRIPTION##

Create a new BSONObj object.

##PARAMETERS##

| Name | Type     | Default | Description | Required or not |
| ---- | -------- | ------- | ----------- | --------------- |
| json | JSON     | --      | json data   | yes             |

##RETURN VALUE##

On success, returns the new BSONObj object.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Create a new BSONObj object.

```lang-javascript
> var bsonObj = BSONObj( { name: "fang" } )
> bsonObj
{
  "name": "fang"
}
```

