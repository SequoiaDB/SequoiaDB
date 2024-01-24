##NAME##

setActiveLocation - set the "ActivedLocation" of the replication group

##SYNOPSIS##

**rg.setActiveLocation(\<location\>)**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to set the specified location to "ActiveLocation" in the current replication group.

##PARAMETERS##

location ( *string, required* )

Location name.

- The specified location needs to exist in the current replication group.
- When the value is an empty string, it means to delete the "ActiveLocation" of the current replication group.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setActiveLocation()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error. | Check whether the parameter type is correct. |
| -259 | SDB_OUT_OF_BOUND | Required parameters not specified. | Check whether missing required parameters. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

- Set location "GuangZhou" to "ActiveLocation" of replication group "group1".

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setActiveLocation("GuangZhou")
    ```

- Delete "ActiveLocation" from replication group "group1".

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setActiveLocation("")
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md