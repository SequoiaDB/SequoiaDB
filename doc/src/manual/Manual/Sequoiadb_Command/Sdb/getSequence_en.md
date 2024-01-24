##NAME##

getSequence - get the reference of specified sequence in current database

##SYNOPSIS##

**db.getSequence\(\<name\>\)**

##CATEGORY##

Sdb

##DESCRIPTION##

Get the reference of specified sequence in current database.

##PARAMETERS##

name ( *string*, *required* )

Sequence name.

##RETURN VALUE##

When the function executes successfully, it will return the specified sequence object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `getSequence()`:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-324      |SDB_SEQUENCE_NOT_EXIST|The sequence does not exist|Check for sequence existence|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Get the named sequence, and use the returned object to get the sequence next value.

```lang-javascript
> var sequence = db.getSequence("IDSequence")
> sequence.getNextValue()
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/reference/Sequoiadb_command/Global/getLastErrMsg.md
[getLastError]:manual/reference/Sequoiadb_command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
