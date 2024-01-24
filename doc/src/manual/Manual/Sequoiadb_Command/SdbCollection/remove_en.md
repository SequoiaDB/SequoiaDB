##NAME##

remove - remove records in the collection

##SYNOPSIS##

**db.collectionspace.collection.remove\(\[cond\], \[hint\], \[options\]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to remove records in the collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | -------- | ------ | -------- |
| cond   | object| Selection condition. When it is empty, remove all records. When it is not empty, remove the records that meet the conditions. | required |
| hint   | object| Specify an access plan. | not |
| options| object| Options. For more details, refer to the description of options.| not |

options:

| Name         | Type| Description   | Defaults |
| --------------- | -------- | ------------------- | ------ |
| JustOne         | boolean     | When it is true, only one eligible record will be updated.<br>When it is false, all eligible records will be updated.| false  |

> **Note:**
>
> - The usage of the parameters cond and hint are used in the same way as [find()][find].
> - When JustOne is true, it can only be executed on a single partition and a single subtable.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get a list of successful deletion through this object, and the field descriptions are as follows:

| Name | Type | Description |
|--------|------|------|
| DeletedNum | int64 | Number of records successfully deleted. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `remove()` function are as follows:
  
| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -348     | SDB_COORD_DELETE_MULTI_NODES|When the parameter "JustOne" is true, delete records across multiple partitions or subtables. | Modify the matching conditions or do not use the parameter "JustOne". |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Remove all records in the collection.

    ```lang-javascript
    > db.sample.employee.remove()
    ```

- Remove records matching the cond condition according to the access plan. For example, the following operation traverses the records in the collection according to the index named "myIndex", and removes the records whose "age" field value is greater than or equal to 20 from the traversed records.

    ```lang-javascript
    > db.sample.employee.remove({age: {$gte: 20}}, {"": "myIndex"})
    ```

[^_^]:
    Links
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
