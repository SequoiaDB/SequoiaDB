##NAME##

current - get the record pointed to by the current cursor

##SYNOPSIS##

**cursor.current()**

##CATEGORY##

SdbCursor

##DESCRIPTION##

This function is used to get the record pointed to by the current cursor, more content can refer to [next()][next].

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, if the cursor has a record to return, it will return an object of type BSONObj; Otherwise, it will return an object of type null.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `current()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ 	| -----------	| ------					|
| -29    | SDB_DMS_EOC | When the server returns no records, If user gets the first record through the current() interface, it will fail. | Under any condition, Users should first use the next() interface of the cursor to get the first record. When no record is returned from the server, next() interface will return null instead of throwing a -29 error. |
| -31	 | SDB_DMS_CONTEXT_IS_CLOSE | Context closed. | Confirm whether the query records are 0.	|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

Select the record of "a" is 1 in the collection "employee",  returns the record pointed to by the current cursor.

```lang-javascript
> var cur = db.sample.employee.find({a: 1});
> var obj = null;
> while((obj = cur.next() != null)){
    println("Record is:" + cur.current());
}
Record is:{
"_id": {
    "$oid": "60470a4db354306ff89cd355"
},
"a": 1
}
```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[next]:manual/Manual/Sequoiadb_Command/SdbCursor/next.md