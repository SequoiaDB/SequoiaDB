##NAME##

invalidateCache - clear the cache of the nodes

##SYNOPSIS##

**db.invalidateCache( [options] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to clear the cache of the catalog, data or coord nodes.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| options | json   | Specify [command positional parameter][location]. When null, clear the cache of the catalog, data or coord nodes. | Not |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Clear the cache of the current coord node.

```lang-javascript
> db.invalidateCache( { Global: false } )
```

* Clear the cache of the current coord node and 'group1' group's nodes.

```lang-javascript
> db.invalidateCache( { GroupName: 'group1' } )
```



[^_^]:
    links
[location]:manul/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md