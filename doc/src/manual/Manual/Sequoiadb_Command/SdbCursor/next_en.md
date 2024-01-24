##NAME##

next - get the next record pointed to by the current cursor

##SYNOPSIS##

**cursor.next()**

##CATEGORY##

SdbCursor

##DESCRIPTION##

This function is used to get the next record pointed to by the current cursor, more content can refer to [current()][current].

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, if the cursor has a record to return, it will return an object of type BSONObj; Otherwise, it will return an object of type null.

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

The common exceptions of `next()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ 		| ------   | ------------	| ------					|
| -31			| SDB_DMS_CONTEXT_IS_CLOSE | Context closed| Confirm whether the query record is 0.	|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

Select the records with "age" greater than 8 under collection "employee", return the next record pointed to by the current cursor.

```lang-javascript
> var cur = db.sample.employee.find({age: {$gt: 8}})
> var obj = cur.next();
> if (obj == null) {
      println ("No record!");
  } else {
        println ("Record is:" + obj);
    }
  Record is:{
    "_id":{
      "$oid": "60470a4db354306ff89cd355"
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
[current]:manual/Manual/Sequoiadb_Command/SdbCursor/current.md