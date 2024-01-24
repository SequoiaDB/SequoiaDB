##NAME##

setActiveLocation - set "ActiveLocation" for all replication groups in the domain

##SYNOPSIS##

**domain.setActiveLocation(\<location\>)**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to set the "ActiveLocation" of all replication groups at the same time in the current domain.

##PARAMETERS##

location ( *string, required* )

Location name.

- The specified location needs to exist in all replication groups contained in the domain.
- When the value is an empty string, it means delete "ActiveLocation" of all replication groups in the domain.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setActiveLocation()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error. | Check whether the parameter type is correct. |
| -259 | SDB_OUT_OF_BOUND | Required parameters not specified. | Check whether missing required parameters. |
| -264 | SDB_COORD_NOT_ALL_DONE | Thespecified location does not exist in the partial replication group. | Check whether the location exists in the replication group where the setup failed. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

- Domain "mydomain" contains replication "group1" and "group2", set "ActiveLocation" to "GuangZhou" for all replication groups in the domain.

    ```lang-javascript
    > var domain = db.getDomain("mydomain")
    > domain.setActiveLocation("GuangZhou")
    ```

    Check whether the setting is successful.

    ```lang-javascript
    > db.list(SDB_LIST_GROUPS, {}, {ActiveLocation: "", GroupName: ""})
    ...
    {
      "ActiveLocation": "GuangZhou",
      "GroupName": "group1"
    }
    {
      "ActiveLocation": "GuangZhou",
      "GroupName": "group2"
    }
    ...
    ```

- Delete "ActiveLocation" from all replication groups in the domain.

    ```lang-javascript
    > domain.setActiveLocation("")
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md