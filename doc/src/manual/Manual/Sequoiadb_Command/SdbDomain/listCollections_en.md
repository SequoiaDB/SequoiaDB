##NAME##

listCollections - enumerate the collection information in the domain

##SYNOPSIS##

**domain.listCollections()**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to enumerate all collection information in the specified domain.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get collection information in the domain through this object. For field descriptions, refer to [collections_list][collections_list].

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Get the collection under the specified domain.

```lang-javascript
> domain.listCollections()
{
    "Name": "sample.employee" 
}
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[collections_list]:manual/Manual/List/SDB_LIST_COLLECTIONS.md