##NAME##

truncateLob - truncate LOB in the collection

##SYNOPSIS##

**db.collectionspace.collection.truncateLob\(\<oid\>, \<length\>\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to truncate LOB in the collection.

##PARAMETERS##

- oid ( *string, required* )

    Unique identifier of LOB.

- length（ *number, required* )

    The length of the LOB after truncation.

    - When the value of length is less than 0, output error message.
    - When the value of length is greater than the length of the LOB, no truncation occurs.


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

Specify a LOB whose oid is "5435e7b69487faa663000897" and truncate its length to 0.

```lang-javascript
> db.sample.employee.truncateLob("5435e7b69487faa663000897", 0)
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

