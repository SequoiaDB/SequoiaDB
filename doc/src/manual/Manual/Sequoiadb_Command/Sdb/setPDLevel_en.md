##NAME##

setPDLevel -  set the diagnostic log level for the node dynamically

##SYNOPSIS##

**db.setPDLevel(\<level\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to set the diagnostic log level for the node dynamically.

##PARAMETERS##

- level ( *number, required* )

    Specify the log level. The value ranges from 0 to 5, which is respectively corresponding to:

    - 0: SEVERE
    - 1: ERROR
    - 2: EVENT
    - 3: WARNING
    - 4: INFO
    - 5: DEBUG

- options ( *object, optional* )

    Specify [command position parameters][location].

> **Note:**
>
> * When the log level is incorrect, the default setting is WARNING.
> * When there is no positional parameter, the default is only valid for its own node.
> * The setting parameters will not be persisted.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

* Set the log level of the current node to DEBUG.

    ```lang-javascript
    // Connection node
    > db = new Sdb("localhost", 11810)
    > db.setPDLevel(5)
    ```

* Set the log level of node 1000 to INFO.

    ```lang-javascript
    // Connection node
    > db = new Sdb("localhost", 11810)
    > db.setPDLevel(4, {NodeID: 1000})
    ```

* Set the log level of all nodes to EVENT.

    ```lang-javascript
    // Connection node
    > db = new Sdb("localhost", 11810)
    > db.setPDLevel(3, {Global:true})
    ```


[^_^]:
   links
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md