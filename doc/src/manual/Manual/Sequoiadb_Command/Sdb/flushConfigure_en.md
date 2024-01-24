##NAME##

flushConfigure - flush the configuration in the node memory to the configuration file

##SYNOPSIS##

**db.flushConfigure([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to flush the configuration in the node memory to the configuration file.

##PARAMETERS##

options ( *object, optional* )

Through the options parameter, users can set the configuration filter type and [command position parameter][location]：

- Type ( *number* )：Configure the filter type, the default value is 3.

    The values are as follows:

    - 0：All configurations.
    - 1：Shield unmodified hidden parameters.
    - 2：Shield all unmodified parameters.
    - 3：Shield unconfigured parameters.

    Format: `Type: 3`

- Location Elements：Command position parameter item.

    Format: `GroupName: "db1"`

> **Note:**
>
> * When the configuration filter type is incorrect, the default setting is 3.
> * When there is no position parameter, the default is only valid for the node itself.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Flush the database configuration.

```lang-javascript
> db.flushConfigure({Global: true})
```


[^_^]:
   links
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md