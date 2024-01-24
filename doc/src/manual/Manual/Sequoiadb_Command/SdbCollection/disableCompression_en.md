##NAME##

disableCompression - modify the properties of the collection to turn off the compression function

##SYNOPSIS##

**db.collectionspace.collection.disableCompression()**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to modify the properties of the collection to turn off the compression function.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `disableCompression()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Options are not currently supported| Check the properties of the current collection, if it is a partition collection, users cannot modify the properties related to the partition.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

Create a compressed collection, and then turn off the compression function of the collection.

```lang-javascript
> db.sample.createCL('employee', {CompressionType: 'snappy'})
> db.sample.employee.disableCompression()
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md