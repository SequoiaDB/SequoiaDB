##名称##

start - 后台执行 Shell 指令

##语法##

**cmd.start(\<cmd\>,[args],[timeout],[useShell])**

##类别##

Cmd

##描述##

后台执行 Shell 指令。

##参数##

| 参数名   | 参数类型 | 默认值 | 描述            | 是否必填 |
| -------- | -------- | ------ | --------------- | -------- |
| cmd      | string   | ---    | Shell 命令名称  | 是       |
| args     | string   | 空     | 命令参数        | 否       |
| timeout  | int      | 0      | 设置超时时间    | 否       |
| useShell | int      | 1      | 是否使用 /bin/sh 解析执行命令，默认使用 | 否       |

##返回值##

返回执行该命令所对应的线程号。

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

* 执行命令行指令

    ```lang-javascript
    > cmd.start( "ls", "/opt/trunk/test" )
    28340 
    ```

* 获取命令执行的返回结果

    ```lang-javascript
    > cmd.getLastOut()
    test1
    test2
    test3 
    ```
