##NAME##

getCL - get the object reference of the specified collection in the current collection space

##SYNOPSIS##

**db.collectionspace.getCL(\<name\>)**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to obtain the object reference of the specified collection in the current collection space.

##PARAMETERS##

name ( *string, required* )

Collection name.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCollection.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `getCL()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | Collection does not exist | Check whether the collection exists.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.0 and above

##EXAMPLES##

Returns the reference of the collection "employee" in the collection space "sample".

```lang-javascript
> var cl = db.sample.getCL("employee")
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md