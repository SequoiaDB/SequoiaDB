##NAME##

disable - disable the recycle bin

##SYNOPSIS##

**db.getRecycleBin().disable()**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to disable the recycle bin. When disabled, operation such as dropping a collection space will not generate recycle bin items.

##PARAMETERS##

None.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Disable the recycle bin.

```lang-javascript
> db.getRecycleBin().disable()
```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
