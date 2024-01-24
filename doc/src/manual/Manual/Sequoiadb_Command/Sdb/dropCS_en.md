##NAME##

dropCS - Delete a specified collection space.

##SYNOPSIS##

**db.dropCS(\<name\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

Delete a specified collection space.

##PARAMETERS##

* `name` ( *String*， *Required* )

    Collection space name.

* `options` ( *Object*， *Optional* )

    The options for dropping the collection space, could be a combination of 
    the following options:

    1. `EnsureEmpty` ( *Bool* ): Ensure the collection space is empty or not, default to be false.

        * true: when collection space contains collection, dropping is canceled and return with error code -275 ; when collection space does not contain any collection, dropping is proceed.
        * false: dropping is proceed whether collection space contains collection or not.

        Format: `EnsureEmpty:true|false`

    2. `SkipRecycleBin` ( *Bool* ):  Whether to disable the [recycle bin][recycle_bin]. The default is false.

        * true: When deleting a collection space, the corresponding recycle bin item will  not be generated.
        * false: Determine whether to enable the recycle bin mechanism according to the value of the field "[Enable][getDetail]".

        Format: `SkipRecycleBin:true|false`

##RETURN VALUE##

On success, the specified collection space is dropped.

On error, exception will be thrown.

##ERRORS##

The exceptions of `dropCS()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -34 | SDB_DMS_CS_NOTEXIST | The collection space is not exist. | Check whether collection space is exist or not. |
| -275 | SDB_DMS_CS_NOT_EMPTY | The collection space is not empty. | Check whether the "EnsureEmpty" option is enabled or not. |
| -386 | SDB_RECYCLE_FULL | Recycle bin is full. | Check if recycle bin is full and manually clean the recycle bin [dropItem()][dropItem] or [dropAll()][dropAll]. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Drop an exist collection space named by "foo".

    ```lang-javascript
    > db.dropCS("foo")
    Takes 0.003132s.
    ```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[recycle_bin]:manual/Distributed_Engine/Maintainance/recycle_bin.md
[dropItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md
[dropAll]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md
