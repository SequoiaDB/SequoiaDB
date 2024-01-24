##NAME##

exec - execute the select statement of SQL

##SYNOPSIS##

**db.exec(\<select sql\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to execute the select statement of SQL.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of search result through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Find all the records with age = 20 from the "sample.employee".

```lang-javascript
> db.exec("select * from sample.employee where age = 20")
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md