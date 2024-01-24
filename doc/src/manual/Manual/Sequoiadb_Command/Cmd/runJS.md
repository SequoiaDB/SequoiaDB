##名称##

runJS - 远程执行 JavaScript 代码

##语法##

**cmd.runJS(\<code\>)**

##类别##

Cmd

##描述##

远程执行 JavaScript 代码。

> **Note:** 

> runJS() 必须被远程 Command 对象调用

##参数##

| 参数名    | 参数类型 | 默认值 | 描述            | 是否必填 |
| --------- | -------- | ------ | --------------- | -------- |
| code      | string   | ---    | JavaScript 代码 | 是       |

##返回值##

返回代码执行结果。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 新建一个远程 Command 对象 （建立远程连接详细可参考[远程连接](manual/Manual/Sequoiadb_Command/Remote/Remote.md)）

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    > var cmd = remoteObj.getCmd()
    ```

* 执行 JavaScript 代码

    ```lang-javascript
    > cmd.runJS( "1+2*3" )
    7
    ```
