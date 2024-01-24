##NAME##

listCollectionSpaces - list the collection space contained in the domain

##SYNOPSIS##

**domain.listCollectionSpaces()**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to list the collection space contained in the specified domain.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of collection space information contained in the domain through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

List the collection space information contained in the specified domain.

```lang-javascript
> var domain = db.getDomain('mydomain')
> domain.listCollectionSpaces()
{
  "Name": "sample1"
}
{
  "Name": "sample2"
}
```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[collectionspaces_list]:manual/Manual/List/SDB_LIST_COLLECTIONSPACES.md
