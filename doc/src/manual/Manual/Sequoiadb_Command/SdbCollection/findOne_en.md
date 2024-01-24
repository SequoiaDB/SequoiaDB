##NAME##

findOne - query a record that meets the conditions

##SYNOPSIS##

**db.collectionspace.collection.findOne\(\[cond\], \[sel\]\)**

**db.collectionspace.collection.findOne([cond], [sel]).hint([hint])**

**db.collectionspace.collection.findOne([cond], [sel]).skip([skipNum]).limit([retNum]).sort([sort])**

**db.collectionspace.collection.findOne([SdbQueryOption])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to return a record that meets the query conditions, it has the same usage as the [find()][find] method.

##PARAMETERS##

For parameter description, refer to [find()][find] method.

##RETURN VALUE##
  
When the function executes successfully, it will return an object of type SdbQuery. Users can get the result  set of the query through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `findOne()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -2 | SDB_OOM | No memory available| Check the settings and usage of physical memory and virtual memory.|
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameters are filled in correctly.|
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist| Check whether the collection space exists.|
| -23 | SDB_DMS_NOTEXIST| Collection does not exist | Check whether the collection exists.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

- Query all records without specifying the "cond" and "sel" fields.

    ```lang-javascript
    > db.sample.employee.findOne()
    ```

- Query the records matching the conditions by setting the content of the cond parameter. For example, the following operation returns the records in the collection "employee" whose age field value is greater than 25 and the name field value is "Tom".

    ```lang-javascript
    > db.sample.employee.findOne({age: {$gt: 25}, name: "Tom"})
     ```

- Specify the returned field name by setting the content of the sel parameter. For example, there are records {age: 25, type: "system"} and {age: 20, name: "Tom", type: "normal"}, the following operation returns the age field and name field of the record.

    ```lang-javascript
    > db.sample.employee.findOne(null, {age: "", name: "" })
        {
            "age": 25,
            "name": ""
        }
    ```

- Use the index "ageIndex" to traverse the records in the "age" field under the collection employee, and return the records.

    ```lang-javascript
    > db.sample.test.findOne({age: {$exists: 1}}).hint({"": "ageIndex"})
    {
            "_id": {
            "$oid": "5812feb6c842af52b6000007"
            },
            "age": 10
    }
    ```

- Return the records in the collection "employee" whose "age" field value is greater than 20 (for example, use [$gt][gt] query) and set to only return the "name" and "age" fields of the records, and sort the age and name fields in ascending order of the "age" field value.

    ```lang-javascript
    > db.sample.employee.findOne({age: {$gt: 20}}, {age: "", name: ""}).sort({age: 1})
    ```

    Through the findOne() method, users can choose any field name they want to return. In the above example, user have chosen to return the "age" and "name" fields of the records. At this time, when using the sort() method, only the "age" or "name" fields of the records can be sorted. And if user choose to return all the fields of the record, that is, without setting the content of the "sel" parameter of the findOne method, then sort() can sort any field.

- Specify an invalid sort field.

    ```lang-javascript
    > db.sample.employee.findOne({age: {$gt: 20}}, {age: "", name: ""}).sort({"sex": 1})
    ```

    Because the "sex" field does not exist in the "sel" option {age: "", name: ""} of the findOne() method, the sort field {"sex": 1} specified by sort() will be ignored.

[^_^]:
    Links
[overview]:manual/Manual/Operator/Match_Operator/Readme.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[QueryOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbQueryOption.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[gt]:manual/Manual/Operator/Match_Operator/gt.md