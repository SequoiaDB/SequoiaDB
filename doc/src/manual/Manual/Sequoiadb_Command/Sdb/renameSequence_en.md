##NAME##

renameSequence - rename the specified sequence

##SYNOPSIS##

**db.renameSequence\(\<oldname\>, \<newname\>\)**

##CATEGORY##

Sdb

##DESCRIPTION##

Rename the specified sequence.

##PARAMETERS##

+ oldname ( *string*, *required* )

    The name of the sequence.

+ newwname ( *string*, *required* )

    The new name of the sequence.

##RETURN VALUE##

When the function executes successfully, return void.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `renameSequence()`:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-324      |SDB_SEQUENCE_NOT_EXIST|The sequence does not exist|Check for sequence existence|
|-323      |SDB_SEQUENCE_EXIST|The new sequence name has been used|Change the new name|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Rename the sequence from 'IDSequence' to 'ID_SEQ'.

```lang-javascript
> db.renameSequence("IDSequence", "ID_SEQ")
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/reference/Sequoiadb_command/Global/getLastErrMsg.md
[getLastError]:manual/reference/Sequoiadb_command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
