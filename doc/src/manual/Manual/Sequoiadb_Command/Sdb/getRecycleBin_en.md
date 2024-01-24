##NAME##

getRecycleBin - get the reference to the recycle bin

##SYNOPSIS##

**db.getRecycleBin()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the reference to the recycle bin. Through this reference, users can operate on the recycle bin.

##PARAMETERS##

None.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbRecycleBin.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

1. Get the reference to the recycle bin.

    ```lang-javascript
    > var recycle = db.getRecycleBin()
    ```

2. View information on the recycle bin with this reference.

    ```lang-javascript
    > recycle.getDetail()
    {
      "Enable": true,
      "ExpireTime": 4320,
      "MaxItemNum": 1000,
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
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md
