##NAME##

createSpareRG - create a hot spare group

##SYNOPSIS##

**db.createSpareRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a hot spare group which users can manage [hot spare nodes][spare].

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object type of SdbReplicaGroup.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

Create a hot spare group

```lang-javascript
> db.createSpareRG()
```


[^_^]:    
    links
[spare]:manual/Distributed_Engine/Maintainance/hot_spare.md
[getLastErrMsg]:manual/reference/Sequoiadb_command/Global/getLastErrMsg.md
[getLastError]:manual/reference/Sequoiadb_command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
