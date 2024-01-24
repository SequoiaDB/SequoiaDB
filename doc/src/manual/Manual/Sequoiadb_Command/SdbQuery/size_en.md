
##NAME##

size - Get the number of records from the current cursor to the final cursor.

##SYNOPSIS##

***query.size()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the number of records from the current cursor to the final cursor.

>**Note:**  

>The result that size() return is influenced by skip() and limit().

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the number of records.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 20 under the collection, employee, and get the number of records from the current cursor to the final cursor.

```lang-javascript
> db.sample.employee.find( { age: { $gt: 20 } } ).size()
1
```