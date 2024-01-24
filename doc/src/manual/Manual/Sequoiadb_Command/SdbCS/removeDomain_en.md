##NAME##

removeDomain - remove the domain of the collection space

##SYNOPSIS##

**db.collectionspace.removeDomain()**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to remove the domain of the collection space.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

Create a collection space and specify a domain, and then remove the collection space from the domain.

```lang-javascript
> db.createCS('sample', {Domain: 'domain'})
> db.sample.removeDomain()
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md