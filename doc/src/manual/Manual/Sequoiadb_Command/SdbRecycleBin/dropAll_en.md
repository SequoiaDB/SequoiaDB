##NAME##

dropAll - drop all items from recycle bin

##SYNOPSIS##

**db.getRecycleBin().dropAll([options])**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to drop all items from recycle bin.

##PARAMETERS##

options ( *object, optional* )

Other optional parameters can be set through "options":

- Async ( *boolean* ): Whether to use asynchronous mode to delete recycle bin items. The default is false, which means not to use asynchronous mode.

    Format: `Async: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Drop all items from recycle bin.

```lang-javascript
> db.getRecycleBin().dropAll()
```

[^_^]:
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
