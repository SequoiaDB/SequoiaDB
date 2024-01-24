##NAME##

sync - persist data and logs to disk

##SYNOPSIS##

**db.sync([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to persist data and logs to disk.

##PARAMETERS##

options ( *object, optional* )

Through the options parameter, users can set the depth brushing, blocking mode, designated collection space and [command position parameter][location]：

- Deep ( *number* ): Whether to open the deep brush. The default value is 1.

    The values are as follows:

    - 0: not open
    - 1: open
    - -1: that the server-side default configuration is used

    Deep value is compatible with boolean type.

    Format: `Deep: 1`

- Block ( *boolean* ): Whether to block all change operations during persistence. The default value is false.

    Format: `Block: false`

- CollectionSpace ( *string* ): Specify the persistent collection space name.

    If this parameter is specified, only the collection space will be persisted, otherwise all collection spaces and log files will be persisted.

    Format: `CollectionSpace: "sample"`

-  Location Elements: Command position parameter item.
  
    Format: `GroupName: "db1"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `sync()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -67    | SDB_BACKUP_HAS_ALREADY_START | Conflict with offline backup task. | Turn off 'blocking mode' and try again. |
|-149    | SDB_REBUILD_HAS_ALREADY_START | Conflict with local reconstruction task. | Turn off 'blocking mode' and try again. |
| -148   | SDB_DMS_STATE_NOT_COMPATIBLE | Conflict with other blocking operations(such as other sync operations). | Turn off 'blocking mode' and try again. |
| -34    | SDB_DMS_CS_NOTEXIST | Specified collection space does not exist. | Confirm whether the collection space exists. |	

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.8 and above

##EXAMPLES##

- Deeply persist all the collextion space and logs of the whole system.

    ```lang-javascript
    > db.sync()
    ```

- Deeply persist the specified collection space "sample".

    ```lang-javascript
    > db.sync({CollectionSpace: "sample"})
    ```

- Perform deep and blocking persistence for the specified data group "group1".

    ```lang-javascript
    > db.sync({GroupName: "group1", Block: true})
    ```


[^_^]:
   links
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md