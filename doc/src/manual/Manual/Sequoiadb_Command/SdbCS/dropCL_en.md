##NAME##

dropCL - drop the specified collection in the current collection space

##SYNOPSIS##

**db.collectionspace.dropCL(\<name\>, [options])**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to delete the specified collection in the current collection space.

##PARAMETERS##

* name ( *string, required* )

    Collection name.

* options ( *object, optional* )

    Other optional parameters can be set through "options":

    - SkipRecycleBin ( *boolean* )：Whether to disable the [recycle bin][recycle_bin]. The default is false, which means whether to enable the recycle bin mechanism according to the value of the field "[Enable][getDetail]".

        The value of this parameter is true, which means that the corresponding recycle bin item will be not be generated when the collection is deleted.

        Format: `SkipRecycleBin: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `dropCL()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | Collection does not exist. | Check whether the collection exists.|
| -386 | SDB_RECYCLE_FULL | Recycle bin is full. | Check if recycle bin is full and manually clean the recycle bin [dropItem()][dropItem] or [dropAll()][dropAll]. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.0 and above

##EXAMPLES##

Delete the collection "employee" under the collection space "sample".

```lang-javascript
> db.sample.dropCL("employee")
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
