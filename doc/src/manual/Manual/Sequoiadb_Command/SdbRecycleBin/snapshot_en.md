##NAME##

snapshot - get the snapshot of items in recycle bin

##SYNOPSIS##

**db.getRecycleBin().snapshot([cond], [sel], [sort])**
**db.getRecycleBin().snapshot([SdbSnapshotOption])**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to get the snapshot of items in recycle bin.

##PARAMETERS##

* cond ( *object, optional* )

    The condition to match items in recycle bin.

* sel ( *object, optional* )

    The selector to select fields of items in recycle bin.

* sort ( *object, optional* )

    The order to sort the items in recycle bin, the value are as follows:

    - 1: Ascending
    - -1: Descending

* SdbSnapshotOptions ( *SdbSnapshotOption, optional* )

    Options of the snapshot, the usage method can refer to [SdbSnapshotOption][shotOption].

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a snapshot of recycle bin through this object. For field descriptions, refer to [recycle bin snapshot][SDB_SNAP_RECYCLEBIN].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Get the snapshot of all recycle bin items.

```lang-javascript
> db.getRecycleBin().snapshot()
```

[^_^]:
       Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[shotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[SDB_SNAP_RECYCLEBIN]:manual/Manual/Snapshot/SDB_SNAP_RECYCLEBIN.md
