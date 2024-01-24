##NAME##

updateNodeConfigs - update node configuration information

##SYNOPSIS##

**oma.updateNodeConfigs(\<svcname\>, \<config\>)**

##CATEGORY##

Oma

##DESCRIPTION##

This function is used to update the configuration information of the specified node. After the update, users need to restart the node or use [reloadConf()][reloadConf] to reload the configuration file to make the configuration effective.

##DESCRIPTION##

- svcname ( *number/string, required* )

	The port of the node

- config ( *object, required* )

	Node configuration information. Such as update log size, whether to open the transaction and so on. For detail can refer to [Configuration item parameters][config].

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `updateNodeConfigs()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error | Check if the port number and configuration information are correct |
| -259 | SDB_OUT_OF_BOUND | No node port number or configuration information entered | Enter the node port number or configuration information |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2 and above

##EXAMPLES##

Update the configuration item parameter "diaglevel" of node 11810 to 3.

```lang-javascript
> var oma = new Oma("localhost", 11790)
> oma.updateNodeConfigs(11810, {diaglevel: 3})
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[reloadConf]:manual/Manual/Sequoiadb_Command/Sdb/reloadConf.md
[config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md