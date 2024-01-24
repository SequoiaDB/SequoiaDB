##NAME##

deleteLob - delete the Lob in collection

##SYNOPSIS##

**db.collectionspace.collection.deleteLob\(\<oid\>\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to delete the Lob in collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | -------- | ---- | -------- |
| oid    | string | Unique descriptor for Lob | required |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Delete a LOB with a descriptor of "5435e7b69487faa663000897".

```lang-javascript
> db.sample.employee.deleteLob('5435e7b69487faa663000897')
```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
