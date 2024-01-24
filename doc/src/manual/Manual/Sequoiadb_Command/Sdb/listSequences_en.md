

##NAME##

listSequences - Enumerate sequences information

##SYNOPSIS##

**db.listSequences()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate the sequence information of the current database.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [$LIST_SEQUENCES][LIST_SEQUENCES] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v3.2 and above

##EXAMPLES##

* Show the sequences name of the current database.

```lang-javascript
> db.listSequences()
{
  "Name": "SYS_8589934593_id_SEQ"
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[LIST_SEQUENCES]:manual/Manual/SQL_Grammar/Monitoring/LIST_SEQUENCES.md