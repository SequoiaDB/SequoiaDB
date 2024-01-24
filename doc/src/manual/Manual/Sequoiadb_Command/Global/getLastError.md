
##名称##

getLastError - 获取前一次操作返回的错误码。

##语法##

**getLastError()**

##类别##

Global

##描述##

获取前一次操作返回的错误码。

##参数##

无。

##返回值##

返回 Int32 类型的[错误码](manual/Manual/Sequoiadb_error_code.md)。其中0表示无错误，非0表示错误。

##版本##

v1.0及以上版本。

##示例##

获取前一次操作返回的错误码。

```lang-javascript
> db = new Sdb()
(nofile):0 uncaught exception: -15
> getLastError()
-15
```
