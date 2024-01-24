##名称##

getRecycleBin - 获取回收站的引用

##语法##

**db.getRecycleBin()**

##类别##

Sdb

##描述##

该函数用于获取回收站的引用，通过该引用可以对回收站进行操作。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbRecycleBin 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

1. 获取回收站的引用

    ```lang-javascript
    > var recycle = db.getRecycleBin()
    ```

2. 通过该引用查看回收站的信息

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
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
