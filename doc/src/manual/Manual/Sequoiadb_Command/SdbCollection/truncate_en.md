##NAME##

truncate - delete all data in the collection

##SYNOPSIS##

**db.collectionspace.collection.truncate([options])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to delete all data in the collection (including ordinary documents and LOB data), but it does not affect its metadata. Unlike "remove", which needs to filter targets according to conditions, "truncate" will directly release the data page, which is more efficient than "remove" when clearing the data in the collection (especially with large amounts of data).

> **Note:** 
> 
> If there is an auto-increment field, the field sequence value will be reset after truncate.

##PARAMETERS##

options ( *object, optional* )

Other optional parameters can be set through "options":

- SkipRecycleBin ( *boolean* )：Whether to disable the [recycle bin][recycle_bin]. The default is false, which means whether to enable the recycle bin mechanism according to the value of the field "[Enable][getDetail]".

    The value of this parameter is true, which means that the corresponding recycle bin item will be not be generated when the collection is deleted.

    Format: `SkipRecycleBin: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `truncate()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | Collection does not exist. | Check whether the collection exists.|
| -386 | SDB_RECYCLE_FULL | Recycle bin is full. | Check if recycle bin is full and manually clean the recycle bin [dropItem()][dropItem] or [dropAll()][dropAll]. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Common data and LOB data are inserted into the collection "sample.employee", and the usage of its data pages can be viewed through a snapshot.

    ```lang-javascript
    > db.snapshot(SDB_SNAP_COLLECTIONS)
    {
      "Name": "sample.employee",
      "Details": [
        {
          "GroupName": "datagroup",
          "Group": [
            {
              "ID": 0,
              "LogicalID": 0,
              "Sequence": 1,
              "Indexes": 1,
              "Status": "Normal",
              "TotalRecords": 10000,
              "TotalDataPages": 33,
              "TotalIndexPages": 7,
              "TotalLobPages": 36,
              "TotalDataFreeSpace": 41500,
              "TotalIndexFreeSpace": 103090
            }
          ]
        }
      ]
    }
    ```

- In the above example, the data page is 33, the index page is 7, and the LOB page is 36. The truncate operation is executed below.

    ```lang-javascript
    > db.sample.employee.truncate()
    ```

- Check the data page usage through the snapshot again, the index page is 2 (stored index metadata information), and all the other data pages have been released.

    ```lang-javascript
    > db.snapshot(SDB_SNAP_COLLECTIONS)
    {
      "Name": "sample.employee",
      "Details": [
        {
          "GroupName": "datagroup",
          "Group": [
            {
              "ID": 0,
              "LogicalID": 0,
              "Sequence": 1,
              "Indexes": 1,
              "Status": "Normal",
              "TotalRecords": 0,
              "TotalDataPages": 0,
              "TotalIndexPages": 2,
              "TotalLobPages": 0,
              "TotalDataFreeSpace": 0,
              "TotalIndexFreeSpace": 65515
            }
          ]
        }
      ]
    }
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[recycle_bin]:manual/Distributed_Engine/Maintainance/recycle_bin.md
[dropItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md
[dropAll]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md
