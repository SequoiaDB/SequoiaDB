##NAME##

detachCL - separate the sub-partition collection from the main partition collection

##SYNOPSIS##

**db.collectionspace.collection.detachCL\(\<subCLFullName\>\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to separate the sub-partition collection from the main partition collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | ------ | ------ | ------ |
| partitionName | string | sub-partition name (atomic partition collection name) | required |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Separate the specified sub-partition from the main partition collection.

```lang-javascript
> db.sample.year.detachCL("sample2.January")
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
