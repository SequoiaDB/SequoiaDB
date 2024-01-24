##NAME##

forceSession - terminate the current operation of the specified session 

##SYNOPSIS##

**db.forceSession(\<sessionID\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to terminate the current operation of the specified session. If users do not specify node infomation, the default is to operate on its own session.

##PARAMETERS##

| Name | Type | Description | Required or not |
| ------ | ------ | ------ | ------ |
| sessionID | number | Session number. | required |
| options | object | [Command position parameter][location]. | not |

> **Note:**
>
> * Only user session can be terminated.
> * The sessionID can be obtained by [list()][list] or [snapshot()][snapshot].
> * The options parameter only takes effect on the coordination node.
> * If the terminated session is the current session, the connection will be disconnected and no further operations can be performed.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##


The common expretions of `forceSession()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -62 | SDB_PMD_SESSION_NOT_EXIST | The specified session does not exist. | Check if the specified session exists. |
| -63 | SDB_PMD_FORCE_SYSTEM_EDU | The system session cannot be terminated. | - |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Terminate the session numbered 30 in the current node.

    ```lang-javascript
    // Connection node.
    > db = new Sdb("localhost", 11810)
    // Terminate the specified session.
    > db.forceSession(30)
    ```

* Terminate the session numbered 30 in the node with NodeID 1000.

    ```lang-javascript
    // Connection node.
    > db = new Sdb("localhost", 11810)
    // Terminate the specified session.
    > db.forceSession(30, {NodeID: 1000})
    ```


[^_^]:
   links
[location]:manual/Manual/Sequoiadb_Command/location.md
[list]:manual/Manual/Sequoiadb_Command/Sdb/list.md
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md