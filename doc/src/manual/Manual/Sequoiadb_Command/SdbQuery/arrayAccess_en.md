
##NAME##

arrayAccess - Save the result set in an array and get the record with specified subscript.

##SYNOPSIS##

***query.arrayAccess( \<index\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Save the result set in an array and get the record with specified subscript.

##PARAMETERS##

| Name  | Type | Default | Description     | Required or not |
| ----- | ---- | ------- | --------------- | --------------- |
| index | int  | ---     | array subscript | yes             |

##RETURN VALUE##

On success, return the specified record.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Return the record with array subscript 0.

```lang-javascript
> db.sample.employee.find().arrayAccess(0)
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```
