##NAME##

arrayAccess - save the result set to the array and get the specified subscript record

##SYNOPSIS##

**cursor.arrayAccess(\<index\>)**

**cursor[\<index\>]**

##CATEGORY##

SdbCursor

##DESCRIPTION##

This function is used to get the record of the specified subscript from the array that saves the result set.

##PARAMETERS##

`index` ( *number, required* )

Subscript of the record to be accessed.

##RETURN VALUE##

When the function executes successfully, it will return an object of type String. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

Returns the record with subscript is 0 in the array.

```lang-javascript
> db.sample.employee.find().arrayAccess(0)
{
   "_id": {
   "$oid": "581192bd6db4da2a23000009"
   },
   "a": 9
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md