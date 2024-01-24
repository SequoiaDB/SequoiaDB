##NAME##

listCollections - list the collection name information of the collectionspace

##SYNOPSIS##

**db.collectionspace.listCollections()**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to list all collection name information of the collectionspace.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v3.2 and above

##EXAMPLES##
List the collection name information of the collectionspace "sample".

```lang-javascript
> db.sample.listCollections()
{
  "Name": "sample.a"
}
{
  "Name": "sample.b"
}
{
  "Name": "sample.employee"
}
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md