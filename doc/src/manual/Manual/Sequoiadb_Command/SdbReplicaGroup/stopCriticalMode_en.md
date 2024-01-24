##NAME##

stopCriticalMode - stop Critical mode in the current replication group

##SYNOPSIS##

**rg.stopCriticalMode()**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to stop Critical mode in the current replication group.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Stop Critical mode in replication group "group1".

```lang-javascript
> var rg = db.getRG("group1")
> rg.stopCriticalMode()
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md