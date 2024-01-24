##名称##

getLastRet - 获取上次命令执行返回的错误码

##语法##

**cmd.getLastRet()**

##类别##

Cmd

##描述##

获取上次命令执行返回的错误码。

##参数##

无

##返回值##

返回上次命令执行返回的错误码。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 新建一个 Command 对象

    ```lang-javascript
    > var cmd = new Cmd()
    ```

* 执行命令（ Command 对象执行命令详细可参考[Cmd::run](manual/Manual/Sequoiadb_Command/Cmd/run.md)）

    ```lang-javascript
    > cmd.run( "ls", "/opt/trunk/test" )
    test1
    test2
    test3
    ```

* 获取上次命令执行返回的错误码

    ```lang-javascript
    > cmd.getLastRet()
    0
    ```
