
##NAME##

query - Access query result set with using subscript.

##SYNOPSIS##

***query[ \<index\> ]***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Access query result set with using subscript.

##PARAMETERS##

| Name  | Type | Defulat | Description | Required or not |
| ----- | ---- | ------- | ----------- | --------------- |
| index | int  | ---     | subscript   | yes             |

##RETURN VALUE##

On success, return the record of the specified subscript.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Return the record with the subscript 0.

```lang-javascript
> var query = db.sample.employee.find()
> println( query[0] )
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```