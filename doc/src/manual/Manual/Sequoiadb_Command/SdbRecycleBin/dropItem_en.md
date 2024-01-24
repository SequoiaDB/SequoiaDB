##NAME##

dropItem - drop specified item from recycle bin

##SYNOPSIS##

**db.getRecycleBin().dropItem(\<recycleName\>, [recursive], [options])**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to drop specified item from recycle bin.

##PARAMETERS##

- recycleName ( *string, required* )

    The name of recycle bin item to be dropped.

- recursive ( *boolean, optional* )

    When dropping item of collection space, whether to drop the collection recycle bin item associated with the collection space. The default is false, which means that the associated item will not be drop and an error will be return.

- options ( *object, optional* )

    Other optional parameters can be set through "options":

    - Async ( *boolean* ): Whether to use asynchronous mode to drop recycle bin items. The default is false, which means not to use asynchronous mode.

        Format: `Async: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `dropItem()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -384 | SDB_RECYCLE_ITEM_NOTEXIST | The recycle bin item does not exist. | Check whether the item exists or not. |
| -385 | SDB_RECYCLE_CONFLICT | The recycle bin item has conflicts. | If the operation is to "drop the recycle bin item of the collection space type", users need to specify the parameter "recursive" as true. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

1. List currently existing recycle bin items.

    ```lang-javascript
    > db.getRecycleBin().list()
    {
      "RecycleName": "SYSRECYCLE_8_17179869185",
      "OriginName": "sample1.employee1",
      "Type": "Collection",
      ···
    }
    {
      "RecycleName": "SYSRECYCLE_9_12884901889",
      "OriginName": "sample.employee",
      "Type": "Collection"
      ···
    }
    {
      "RecycleName": "SYSRECYCLE_10_3",
      "OriginName": "sample",
      "Type": "CollectionSpace",
      ···
    }
    ```

2. Drop the collection recycle bin item named "SYSRECYCLE_8_17179869185".

    ```lang-javascript
    > db.getRecycleBin().dropItem("SYSRECYCLE_8_17179869185")
    ```

3. Drop the collection space recycle bin item named “SYSRECYCLE_10_3”.

    ```lang-javascript
    > db.getRecycleBin().dropItem("SYSRECYCLE_10_3")
    ```

    Because there is a collection recycle bin item associated with the collection space "sample" in the recycle bin, the drop operation reports an error.

    ```lang-text
    (shell):1 uncaught exception: -385
    Recycle bin item conflicts:
    Failed to drop collection space recycle item [origin sample, recycle SYSRECYCLE_10_3], there are recursive collection recycle items inside
    ```

    Re-execute the drop operation and specify the parameter "recursive" as true.

    ```lang-javascript
    > db.getRecycleBin().dropItem("SYSRECYCLE_10_3", true)
    ```

4. Confirm that the item was dropped successfully.

    ```lang-javascript
    > db.getRecycleBin().list()
    ```

[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
