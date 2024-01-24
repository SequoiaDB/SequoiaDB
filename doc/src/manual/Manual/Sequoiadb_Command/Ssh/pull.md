##名称##

pull - 从远程主机复制文件到本地

##语法##

**pull( \<remote_file\>, \<local_file\>, \[mode\] )**

##类别##

Ssh

##描述##

从远程主机复制文件到本地。

##参数##

| 参数名     | 参数类型 | 默认值 | 描述         | 是否必填 |
| ---------- | -------- | ------ | ------------ | -------- |
| local_file | string   | ---    | 源文件路径   | 是       |
| dst_file   | string   | ---    | 目标文件路径 | 是       |
| mode       | int      | 0640   | 设置文件权限 | 否       |

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。



常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 使用 SSH 方式连接主机，假设本地主机 IP 地址是“192.168.20.71”

    ```lang-javascript
    > var ssh = new Ssh( "192.168.20.72", "sdbadmin", "sdbadmin", 22 )
    ```

* 从远程主机复制文件到本地。

    ```lang-javascript
    > ssh.pull( "/opt/sequoiadb/remote_file", "/opt/sequoiadb/local_file" )
    ```
