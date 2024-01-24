##NAME##

count - count the total number of eligible records in the current collection

##SYNOPSIS##

**db.collectionspace.collection.count([cond])**

**db.collectionspace.collection.count([cond]).hint([hint])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to count the total number of eligible records in the current collection, users can specify the index used by the query through hint.

##PARAMETERS##

The usage of the parameters `cond` and `hint` is used in the same way as [find()][find].

##RETURN VALUE##

When the function executes successfully, it will return an object of type CLCount. Users can get the total number of records that meet the conditions through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `count()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameters are filled in correctly.|
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist.| Check whether the collection space exists.|
| -23 | SDB_DMS_NOTEXIST| Collection does not exist. | Check whether the collection exists.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.0 and above

##EXAMPLES##

- Count the number of records in the collection "sample.employee", without specifying the parameter "cond".

    ```lang-javascript
    db.sample.employee.count()
    ```
- Count the number of records that the value of the "name" field is "Tom" and the value of the "age" field is greater than 25.

    ```lang-javascript
    > db.sample.employee.count({name: "Tom", age: {$gt: 25}})
    ```

- Count the number of records that the value of the "name" field is "Tom" and the value of the "age" field is greater than 25, using the index "nameIdx".

    ```lang-javascript
    > db.sample.employee.count({name: "Tom", age: {$gt: 25}}).hint({"": "nameIdx"})
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md