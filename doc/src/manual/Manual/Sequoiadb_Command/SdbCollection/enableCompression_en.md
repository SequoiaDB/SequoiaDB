##NAME##

enableCompression - turn on the compression function of the collection or modify the compression algorithm of the collection

##SYNOPSIS##

**db.collectionspace.collection.enableCompression([options])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to turn on the compression function of the collection or modify the compression algorithm of the collection.

##PARAMETERS##

options ( *object, required* )

Modify the compression algorithm type through the options parameters:

- CompressionType ( *string* ): The compression algorithm type of the collection, the default is "lzw" algorithm. The optional values are as follows:

    - "lzw": Using "lzw" algorithm to compress.
    - "snappy": Using "snappy" algorithm to compress.

    Format: `CompressionType: "lzw" | "snappy" `

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `enableCompression()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Option not currently supported| Check the attributes of the current collection, if it is a partitioned collection, user cannot modify the attributes related to the partition.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

Create a normal collection, and then modify the collection to "snappy" compression.

```lang-javascript
> db.sample.createCL('employee')
> db.sample.employee.enableCompression({CompressionType: 'snappy'})
```


[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md