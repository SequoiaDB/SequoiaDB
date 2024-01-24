##NAME##

dropIdIndex - drop the $id index in the collection

##SYNOPSIS##

**db.collectionspace.collection.dropIdIndex()**

##CATEGORY##

SdbCollection

##DESCRIPTION##

The function is used to drop the $id index in the collection, prohibit the update or delete operations.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `dropIdIndex` function are as follows:

| Error code | Error Type |Description | Solution |
| ---------- | ---------- | ---------- | -------- |
| -47        | SDB_IXM_NOTEXIST | $id index does not exist.                 |          -               |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Drop the $id index in the collection.

```lang-javascript
> db.sample.employee.dropIdIndex()
```


[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
