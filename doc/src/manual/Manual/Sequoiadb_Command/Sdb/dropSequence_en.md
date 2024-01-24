##NAME##

dropSequence - drop the specified sequence in current database

##SYNOPSIS##

**db.dropSequence\(\<name\>\)**

##CATEGORY##

Sdb

##DESCRIPTION##

Drop the specified sequence in current database.

##PARAMETERS##

name ( *string*, *required* )

Sequence name.

##RETURN VALUE##

When the function executes successfully, return void.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `dropSequence()`:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-324      |SDB_SEQUENCE_NOT_EXIST|The sequence does not exist|Check for sequence existence|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Drop the sequence named 'IDSequence'.

```lang-javascript
> db.dropSequence("IDSequence")
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/reference/Sequoiadb_command/Global/getLastErrMsg.md
[getLastError]:manual/reference/Sequoiadb_command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
