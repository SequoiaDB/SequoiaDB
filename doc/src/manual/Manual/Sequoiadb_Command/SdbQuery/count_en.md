
##NAME##

count - Get the number of record matching the criteria.

##SYNOPSIS##

***query.count()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the number of record matching the criteria.

>**Note**:

>The result of count() ignores the effects of skip() and limit().

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the number of record matching the criteria.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Select the record whose age is greater than (with using [$gt](reference/operator/match_operator/gt.md)) 10 in the collection employee and return the number of records.

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).count()
3
```