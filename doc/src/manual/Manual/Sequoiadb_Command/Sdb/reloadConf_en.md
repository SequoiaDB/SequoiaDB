##NAME##

reloadConf - reload the configuration file

##SYNOPSIS##

**db.reloadConf([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to reload the configuration file and make the configuration item take effect dynamically. If the configuration item does not allow dynamic effect, it will be ignored.

##PARAMETERS##

| Name   | Type   | Description | Required or not |
| ------ | ------ | ------ | ------ |
| options | object | [Command position parameter][location] | not |

> **Note:**
>
> When the user does not specify the command position parameter, it will take effect for all nodes by default.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

* Reload the configuration of all nodes.

    ```lang-javascript
    // Connect the coordination node.
    > db = new Sdb("localhost", 11810)
    > db.reloadConf()
    ```

* Reload the configuration of the specified node 1000.

    ```lang-javascript
    // Connect the coordination node.
    > db = new Sdb("localhost", 11810)
    > db.reloadConf({NodeID: 1000})
    ```

[^_^]:
   links
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md