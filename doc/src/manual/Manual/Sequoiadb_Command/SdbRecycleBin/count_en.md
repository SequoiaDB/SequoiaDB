##NAME##

count - count the number of items in recycle bin

##SYNOPSIS##

**db.getRecycleBin().count([cond])**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to count the number of items in recycle bin.

##PARAMETERS##

cond ( *object, optional* )

The condition to match items in recycle bin.

##RETURN VALUE##

When the function executes successfully, return the number of items in recycle bin.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Get count of items in recycle bin.

```lang-javascript
> db.getRecycleBin().count()
```

[^_^]:
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
