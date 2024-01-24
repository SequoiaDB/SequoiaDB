##NAME##

memTrim - Reclaim idle memory for a specific node or all nodes

##SYNOPSIS##

**db.memTrim([mask], [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to reclaim idle memory for a specific node or all nodes. When reclaiming "OSS" memory, if there are many memory fragments, it may cause node process delays, blocking other operations.

##PARAMETERS##

| Name   | Type   | Description | Required or not |
| ------ | ---- | ---- | -------- |
| mask | string | OSS, POOL, TC，ALL，You can use '\|' to combine options, and when not specified, the default is 'OSS'. | not |
| options| object | [Command location parameter] [location]<br>If this parameter is not specified, the update operation takes effect on all nodes by default. | not |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v7.0, v5.6.2 and above

##EXAMPLES##

- Recover the "OSS" idle memory for all nodes.

    ```lang-javascript
    > db.memTrim()
    ```

- Recover the "ALL" memory for all data nodes.

    ```lang-javascript
    > db.memTrim("ALL", {Role: "data"})
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[location]:manual/Manual/Sequoiadb_Command/location.md