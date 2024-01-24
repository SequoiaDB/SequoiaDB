##NAME##

transCommit - commit the transaction

##SYNOPSIS##

**db.transCommit()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to commit the transaction. After the transaction is opened, if the operation performed by a single logical unit of work is normal, and the transaction commit command is executed, the data in the database will be updated.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Execute transaction commit command.

```lang-javascript
> db.transCommit()
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md