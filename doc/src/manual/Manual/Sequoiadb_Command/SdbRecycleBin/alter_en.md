##NAME##

alter - modify the properties of recycle bin

##SYNOPSIS##

**db.getRecycleBin().alter(\<options\>)**

##CATEGORY##

SdbRecycleBin

##DESCRIPTION##

This function is used to modify the properties of recycle bin.

##PARAMETERS##

options ( *object, required* )

The properties of recycle bin can be modified through the "options":

- Enable ( *boolean* ): Whether to enable recycle bin.

    Format: `Enable: true`

- ExpireTime ( *number* ): The expiration time of the items in recycle bin, the unit is minutes, and the value range is [-1, 2^31 -1].

    A value of 0 for this parameter means that the recycle bin mechanism is not used. A value of -1 means that it will never expire.

    Format: `ExpireTime: 2880`

- MaxItemNum ( *number* ): Maximum number of items that can be stored in the recycle bin, the value range is [-1, 2^31 -1].

    A value of 0 for this parameter means that the recycle bin mechanism is not used. A value of -1 means there is no limit to the number of items.        

    Format: `MaxItemNum: 2000`

- MaxVersionNum ( *number* ): Maximum number of duplicates that can be stored in the recycle bin, the value range is [-1, 2^31 -1].

    A value of 0 for this parameter means that the recycle bin mechanism is not used. A value of -1 means there is no limit to the number of duplicate items.   

    Format: `MaxVersionNum: 6`

- AutoDrop ( *number* ): Whether to automatically clean up when the number of items stored in the recycle bin exceeds the limit.

    Format: `AutoDrop: true`

> **Note:**
>
> For a detailed description of each field, please refer to [getDetail()][getDetail].

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Modify the expire time of items in recycle bin to 2880 minutes.

```lang-javascript
> db.getRecycleBin().alter({ExpireTime: 2880})
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md
