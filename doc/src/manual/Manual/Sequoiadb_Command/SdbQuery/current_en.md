
##NAME##

current - Get the record pointed to by the current cursor.

##SYNOPSIS##

***query.current()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the record pointed to by the current cursor.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return rearch result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 20 under the collection, employee, and get the record pointed to by the current cursor.

```lang-javascript
> db.sample.employee.find( { age: { $gt: 20 } } ).current()
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```