##NAME##

getSpareRG - get a reference to the spare group

##SYNOPSIS##

**db.getSpareRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get a reference to the spare group.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbReplicaGroup. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

Get a reference of the "SYSSpare" group.

```lang-javascript
> var rg = db.getSpareRG()
```

[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md