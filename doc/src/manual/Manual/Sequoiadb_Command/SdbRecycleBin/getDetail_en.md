##NAME##

getDetail - get detail information of recycle bin

##SYNOPSIS##

**db.getRecycleBin().getDetail()**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to get detail information of recycle bin.

##PARAMETERS##

None.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get a list of recycle bin details through this object. For field descriptions are as follows:     


| Name | Type | Description |
| ---- | ---- | ---- |
| Enable | boolean | Whether to enable the recycle bin mechanism. The default value is true, which means the recycle bin mechanism is enabled. |
| ExpireTime | number | The expiration time of the recycle bin item, the default value is 4320 minutes (that is, three days). Expired recycle bin items will be dropped.|
| MaxItemNum | number | The maximum number of items that can be stored in the recycle bin, the default value is 100. |
| MaxVersionNum | number | The maximum number of duplicate items that can be stored in the recycle bin, the default value is 2. <br> When multiple items have the same field "originName" or "OriginID", these items are considered duplicates. |
| AutoDrop | boolean | Whether to automatically clean up when the number of items stored in the recycle bin exceeds the limit. The default value is false, which means no automatic cleanup.<br>Automatic cleaning is divided into the following situations:<br>1) When the number of items exceeds the limit of the field "MaxItemNum", the earliest generated item will be automatically dropped.<br>2) When the number of duplicate items exceeds the limit of the field "MaxVersionNum", the earliest version of the duplicate item will be automatically dropped. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Get detail information of recycle bin.

```lang-javascript
> db.getRecycleBin().getDetail()
```

The output is as follows:

```lang-json
{
  "Enable": true,
  "ExpireTime": 4320,
  "MaxItemNum": 100,
  "MaxVersionNum": 2,
  "AutoDrop": false
}
```


[^_^]:
      Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
