##NAME##

dropIndex - drop the specified index in the collection

##SYNOPSIS##

**db.collectionspace.collection.dropIndex\(\<name\>\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to drop the specified [index][index] in the collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | -------- | ---- | -------- |
| name   | string   | Index name and the index name in the same collection must be unique.| required |

> **Note:**
>
> - When deleting an index, the index name must exist in the collection.
> - The index name cannot be an empty string and it contains a dot (.) or dollar sign ($), and the length does not exceed 127B.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Drop the index named "ageIndex" under the collection "sample.employee", assuming the index already exists.

```lang-javascript
> db.sample.employee.dropIndex("ageIndex")
```



[^_^]:
    Links
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
