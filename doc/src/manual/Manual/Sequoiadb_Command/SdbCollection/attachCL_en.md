##NAME##

attachCL - mount sub-partition collection

##SYNOPSIS##

**db.collectionspace.collection.attachCL(\<subCLFullName\>, \<options\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to mount the sub-partition collection under the main partition collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | ------ | ------ | ------ |
| subCLFullName | string | sub-partition collection name (including collection space name)| required |
| options | object |  Partition range, including two fields "LowBound" (the left value of the interval) and "UpBound" (the right value of the interval), for example: `{LowBound: {a: 0}, UpBound: {a: 100}}` indicates the range of the field "a": [0, 100). | required |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `attachCL()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -236   | SDB_INVALID_MAIN_CL|Invalid partition collection | Check whether and the main partition collection information is correct, the main partition collection needs to set the attribute "IsMainCL" to "true". |
| -23    |SDB_DMS_NOTEXIST| Collection does not exist   | Check whether the sub-partition collection exists, if it does not exist, create the cooresponding sub-partition collection. |
| -237   |SDB_BOUND_CONFLICT| The new interval conflicts with the existing interval. |Check the existing interval and modify the scope of the newly added interval.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Mount the sub-partition collection under the specified interval of the main partition collection.

```lang-javascript
> db.sample.employee.attachCL("sample2.January", {LowBound: {date: "20130101"}, UpBound: {date: "20130131"}})
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
