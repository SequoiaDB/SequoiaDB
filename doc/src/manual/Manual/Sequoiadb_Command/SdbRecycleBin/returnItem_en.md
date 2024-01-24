##NAME##

returnItem - return specified item from recycle bin

##SYNOPSIS##

**db.getRecycleBin().returnItem(\<recycleName\>, [options])**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to return specified item from recycle bin.

##PARAMETERS##

* recycleName ( *string, required* )

    The name of recycle bin item to be returned.

* options ( *object, optional* )

    Other optional parameters can be set through "options":

    - Enforced ( *boolean* ): Whether to drop the conflicting collections or collection spaces. The default is false, which means that the conflict will be reported as an error.

        The value of this parameter is true, which means that when restoring the recycle bin item, if there is  collections or collection spaces with the same name or the same uniqueID, the original collection or collection space will be forcibly drop to resolve the conflict.

        Format: `Enforced: true`

##RETURN VALUE##

When the function executes successfully,  it will return an object of type BSONObj. Users can get the collection name or collection space name through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `returnItem()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -384 | SDB_RECYCLE_ITEM_NOTEXIST | The recycle bin item does not exist. | Check whether the item exists or not. |
| -385 | SDB_RECYCLE_CONFLICT | The recycle bin item has conflicts. | Rename the item that needs to be restored by [returnItemToName()][returnItemToName], or specify the parameter "Enforced" as true. |

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

2. Return recycle bin item named "SYSRECYCLE_9_21474836481".

    ```lang-javascript
    > db.getRecycleBin().returnItem("SYSRECYCLE_9_21474836481")
    {
      "ReturnName": "sample.employee"
    }
    ```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[returnItemToName]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/returnItemToName.md
