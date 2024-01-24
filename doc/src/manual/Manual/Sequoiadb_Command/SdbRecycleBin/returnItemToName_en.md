##NAME##

returnItemToName - return specified item to specified name from recycle bin

##SYNOPSIS##

**db.getRecycleBin().returnItemToName(\<recycleName\>, \<returnName\>)**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to return specified item to specified name from recycle bin.

##PARAMETERS##

- recycleName ( *string, required* )

    The name of recycle bin item to be returned.

- returnName ( *string, required* )

    The new name after the project is restored. The specified new name must meet the following requirements:

    - The specified name must meet the naming requirements of the collection space or collection. Restrictions include not exceeding 127 bytes, not being an empty string, not starting with "$" or "SYS", and not containing (.).
    - For collection type recycle bin items, the specified name should be in format `<collection space>.<collection>`, and does not support modifying the original collection space name.

##RETURN VALUE##

When the function executes successfully,  it will return an object of type BSONObj. Users can get the collection name or collection space name through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -384 | SDB_RECYCLE_ITEM_NOTEXIST | The recycle bin item does not exist. | Check whether the item exists or not. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

1. List currently existing recycle bin items.

    ```lang-javascript
    > db.getRecycleBin().list()
    {
      "RecycleName": "SYSRECYCLE_9_21474836481",
      "RecycleID": 9,
      "OriginName": "sample.employee",
      "OriginID": 21474836481,
      "Type": "Collection",
      "OpType": "Drop",
      "RecycleTime": "2022-01-24-12.04.12.000000"
    }
    ```

2. Return recycle bin item named "SYSRECYCLE_9_21474836481", and rename the restored collection to "test".

    ```lang-javascript
    > db.getRecycleBin().returnItemToName( "SYSRECYCLE_9_21474836481", "sample.test" )
    {
      "ReturnName": "sample.test"
    }
    ```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[limit]:manual/Manual/sequoiadb_limitation.md
