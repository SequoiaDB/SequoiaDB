##NAME##

setConsistencyStrategy - set the synchronization consistency strategy of the collection

##SYNOPSIS##

**db.collectionspace.collection.setConsistencyStrategy(\<strategy\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to set the [synchronization consistency strategy][consistency_strategy] of the specified collection.

##PARAMETERS##

strategy ( *number, required* )

This parameter is used to set the synchronization consistency strategy of the specified collection.

The values are as follows:

- 1: Node priority strategy.
- 2: Position majority first strategy.
- 3: Main position majority first strategy.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setConsistencyStrategy()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | The parameter type is wrong or the value does not exist. | Check the current parameter type and value. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

1. Set the synchronous consistency strategy of the collection "sample.employee" to node priority strategy.

    ```lang-javascript
    > db.sample.employee.setConsistencyStrategy(1)
    ```

2. View the synchronization consistency strategy of the current collection.

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CATALOG, {Name: "sample.employee"}, {"ConsistencyStrategy": null})
    {
      "ConsistencyStrategy": 1
    }
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[consistency_strategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md