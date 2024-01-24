
##NAME##

toArray - Return the result set as an array.

##SYNOPSIS##

***query.toArray()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Return the result set as an array.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return result set as an array.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Return the first record of the array.

```lang-javascript
> var arr = db.sample.employee.find().toArray()
> arr[0]
{
  "name": "Alice",
  "age": 19
}
```