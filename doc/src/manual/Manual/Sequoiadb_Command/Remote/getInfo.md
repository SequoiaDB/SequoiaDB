##名称##

getInfo - 获取远程连接对象的对象信息

##语法##

***getInfo()***

##类别##

Remote

##描述##

获取远程连接对象的对象信息。

##参数##

无

##返回值##

返回远程连接对象的对象信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。


常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 新建一个远程连接对象。

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    ```

* 获取远程连接对象的对象信息。

    ```lang-javascript
    > remoteObj.getInfo()
    {
        "hostname": "192.168.20.71",
        "svcname": "11790"
    }
    ```