##名称##

exec - 执行 Shell 指令

##语法##

**exec( \<command\> )**

##类别##

Ssh

##描述##

执行 Shell 指令。

##参数##

| 参数名  | 参数类型 | 默认值 | 描述       | 是否必填 |
| ------- | -------- | ------ | ---------- | -------- |
| command | string   | ---    | Shell 命令 | 是       |

##返回值##

返回命令执行结果。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 使用 SSH 方式连接主机。

    ```lang-javascript
    > var ssh = new Ssh( "192.168.20.71", "sdbadmin", "sdbadmin", 22 )
    ```

* 执行命令行指令。

    ```lang-javascript
    > ssh.exec( "ls /opt/sequoiadb/file" )
    file1
    file2
    file3  
    ```
