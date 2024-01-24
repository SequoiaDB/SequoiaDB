
##NAME##

close - Close the cursor.

##SYNOPSIS##

***query.close()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Close the cursor.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Close the cursor.

```lang-javascript
> var query = db.sample.employee.find()
> query.close()
```

* Return the record with the subscript 0.

```lang-javascript
> query[0]
uncaught exception: -31
Failed to get next
```