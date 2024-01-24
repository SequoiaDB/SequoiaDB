##NAME##

getLob - get LOB in the collection

##SYNOPSIS##

**db.collectionspace.collection.getLob\(\<oid\>, \<filepath\>, \[forced\]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to get LOB in the collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| oid  | string | Unique identifier of LOB. | required |
| filepath | string | The full path of the local file to be written. The file does not need to be created manually. | required |
| forced | boolean | Whether to force the overwriting of existing local files, the default value is "false", which means that overwrite is not forced. | not |

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

Write the LOB whose "oid" is "5435e7b69487faa663000897" to the local file `/opt/mylob.txt`

```lang-javascript
> db.sample.employee.getLob('5435e7b69487faa663000897', '/opt/mylob.txt')
{
  "LobSize": 0,
  "CreateTime": {
    "$timestamp": "2021-11-10-14.15.46.466000"
  }
}
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
