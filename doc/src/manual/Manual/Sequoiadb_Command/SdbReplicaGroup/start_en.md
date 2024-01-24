##NAME##

start - start the current replication group

##SYNOPSIS##

**rg.start()**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to start the current replication group. Only after the replication group is started can nodes be created or other operations can be executed. Use [startRG()][startRG] can start the specified node.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##版本##

v2.0 and above

##EXAMPLES##

Start the replication group "group1".

```lang-javascript
> var rg = db.getRG("group1")
> rg.start()   //Equivalent to db.startRG("group")
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[startRG]:manual/Manual/Sequoiadb_Command/Sdb/startRG.md