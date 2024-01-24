##NAME##

getServiceName - get the server name of the node

##SYNOPSIS##

**node.getServiceName()**

##CATEGORY##

SdbNode

##DESCRIPTION##

This function is used to get the server name of the node.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type String. Users can get the server name of the node through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Get the server name of the node "node".

```lang-javascript
> node.getServiceName()
11800
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
