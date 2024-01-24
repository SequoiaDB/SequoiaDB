##NAME##

insert - insert record into the current collection

##SYNOPSIS##

**db.collectionspace.collection.insert(\<doc|docs\>, [flag])**

**db.collectionspace.collection.insert(\<doc|docs\>, [options])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to insert single or multiple records into the current collection.

##PARAMETERS##

- doc|docs ( *object/array, required* )

    Single or multiple records

- flag ( *number, optional* )

    Flag bit, used to control the behavior and result of insert operation. If this parameter is not specified, the insert operation will not return the content of the field "_id" by default, and an error will be reported when an index key conflict occurs.

    the value is as follows:
	
    - SDB_INSERT_RETURN_ID: After successful insertion, return the content of the field "_id" in the record.
    - SDB_INSERT_CONTONDUP: When an index key conflict occurs, skip this record and continue to insert other records.
    - SDB_INSERT_REPLACEONDUP: When an index key conflict occurs, the new record will overwrite the original record and continue to insert other records.
    - SDB_INSERT_CONTONDUP_ID: When $id index key conflict occurs, skip this record and continue to insert other records.
    - SDB_INSERT_REPLACEONDUP_ID: When $id index key conflict occurs, the new record will overwrite the original record and continue to insert other records.

    >**Note：**
    >
    > - "SDB_INSERT_RETURN_ID" supports specifying with other flags at the same time, multiple values are separated by  "|".
    > - For "SDB_INSERT_CONTONDUP", "SDB_INSERT_REPLACEONDUP", "SDB_INSERT_CONTONDUP_ID" and "SDB_INSERT_REPLACEONDUP_ID" do not support specifying multiple.

- options ( *object* )

    The behavior and result of the insert operation can be controlled through the parameter "options":
 
    - ReturnOID ( *boolean* ): Consistent with the behavior of "SDB_INSERT_RETURN_ID" in the parameter "flag".

        Format: `ReturnOID: true`

    - ContOnDup ( *boolean* ): Consistent with the behavior of "SDB_INSERT_CONTONDUP" in the parameter "flag".

        Format: `ContOnDup: true`

    - ReplaceOnDup ( *boolean* ): Consistent with the behavior of "SDB_INSERT_REPLACEONDUP" in the parameter "flag".

        Format: `ReplaceOnDup: true`

    - ContOnDupID ( *boolean* ): Consistent with the behavior of "SDB_INSERT_CONTONDUP_ID" in the parameter "flag".

        Format: `ContOnDupID: true`

    - ReplaceOnDupID ( *boolean* ): Consistent with the behavior of "SDB_INSERT_REPLACEONDUP_ID" in the parameter "flag".

        Format: `ReplaceOnDupID: true`

    >**Note:**
    >
    > - If the parameter "options" is not specified, the insert operation will not return the content of the field "_id" by default, and an error will be reported when an index key conflict occurs.
    > - For the parameters "ContOnDup", "ReplaceOnDup", "ContOnDupID" and "ReplaceOnDupID" do not support specifying multiple as true at the same time.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get information about the number of successfully inserted records through this object, field descriptions are as follows:

|  Name  | Type | Description |
|--------|------|-------------|
| InsertedNum | int64 | The number of records successfully inserted.(not including records that were overwritten) |
| DuplicatedNum | int64 | The number of records covered due to index key conflicts. |
| LastGenerateID | int64 | The value of the auto-increment field (only displayed when the collection contains [auto-increment][auto-increment]), the return situation is as follows:<br> - When inserting a single record, return the auto-incremented field value corresponding to the record.<br>- When inserting multiple records, only return the increment field value corresponding to the first record.<br>- When there are multiple auto-increment fields, insert a single record and only return the maximum value of all auto-increment fields. <br> - When there are multiple auto-increment fields, insert multiple records and only return the largest auto-increment field value corresponding to the first record. |
| _id | oid | Return the content contained in the field "_id" in the inserted record.(only displayed when the parameter "flag" is "SDB_INSERT_RETURN_ID" or the parameter "ReturnOID" is true)|

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `insert()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameters are filled in correctly. |
| -23 | SDB_DMS_NOTEXIST| Collection does not exist. | Check whether the collection exists. |
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist. | Check whether the collection space exists. |
| -38 | SDB_IXM_DUP_KEY | Index key already exists. | Check whether the index key of the inserted record exists. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

- Insert a record in the collection "sample.employee".

    ```lang-javascript
    > db.sample.employee.insert({name: "Tom", age: 20})
    ```

- Insert multiple records in the collection "sample.employee".

 	```lang-javascript
 	> db.sample.employee.insert([{_id: 20, name: "Mike", age: 15}, {name: "John", age: 25, phone: 123}])
 	```

- Insert multiple records with duplicate _id keys in the collection "sample.employee", and specify the parameter "flag" as "SDB_INSERT_CONTONDUP".

    ```lang-javascript
    
    > db.sample.employee.insert([{_id: 1, a: 1}, {_id: 1, b: 2}, {_id: 3, c: 3}], SDB_INSERT_CONTONDUP)
    > db.sample.employee.find()
    {
      "_id": 1,
      "a": 1,
    }
    {
      "_id": 3,
      "c": 3
    }
    ```

- Insert multiple records in the collection "sample.employee", and specify the parameter "ReturnOID" as true.

    ```lang-javascript
    > db.sample.employee.insert([{a: 1}, {b: 1}], {ReturnOID: true})
    {
        "_id": [
            {
                "$oid": "5bececdf6404b9295a63cacb"
            },
            {
                "$oid": "5bececdf6404b9295a63cacc"
            }
        ]
        "InsertedNum": 2,
        "DuplicatedNum": 0
    }
    ```

- Create an auto-increment field in the collection "sample.employee" and insert a record.

    ```lang-javascript
    > db.sample.employee.createAutoIncrement({Field: "ID"})
    > db.sample.employee.insert({a: 1})
    {
        "InsertedNum": 1,
        "DuplicatedNum": 0,
        "LastGenerateID": 1
    }
    ```
 
[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[auto-increment]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md