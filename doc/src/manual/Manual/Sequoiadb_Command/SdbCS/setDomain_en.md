##NAME##

setDomain - modify the domain of the collection space

##SYNOPSIS##

**db.collectionspace.setDomain(\<options\>)**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to modify the domain of the collection space.

##PARAMETERS##

options ( *object，required* )

Modify the collection space properties through the options parameter:

- Domain ( *string* )：The domain of the CollectionSpace.

    - The data of the CollectionSpace must be distributed on the group of the newly designated domain.

    Format：`Domain: <domain>`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

Create a CollectionSpace, and then specify a domain for the CollectionSpace.

```lang-javascript
> db.createCS('sample')
> db.sample.setDomain({Domain: 'domain'})
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md