##名称##

getInfo - 获取 Command 对象的对象信息

##语法##

**cmd.getInfo()**

##类别##

Cmd

##描述##

获取 Command 对象的对象信息。

##参数##

无

##返回值##

返回 Command 对象的对象信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 新建一个远程 Command 对象（建立远程连接详细可参考[远程连接](manual/Manual/Sequoiadb_Command/Remote/Remote.md)）

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    > var cmd = remoteObj.getCmd()
    ```

* 获取 Command 对象的对象信息

    ```lang-javascript
    > cmd.getInfo()
    {
        "type": "Cmd",
        "hostname": "192.168.20.71",
        "svcname": "11790",
        "isRemote": true
    }
    ```