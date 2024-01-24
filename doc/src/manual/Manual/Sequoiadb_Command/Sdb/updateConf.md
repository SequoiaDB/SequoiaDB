##名称##

updateConf - 更新指定的节点配置

##语法##

**db.updateConf(\<config\>, [options])**

##类别##

Sdb

##描述##

该函数用于更新指定的节点配置。生效类型为“在线生效”的配置，更新后立即生效；生效类型为“重启生效”的配置，需重启节点后生效。各配置的生效类型可参考[参数说明][parameter]。如果需要设定非官方配置项，必须打开`Force`选项。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| config | object | [节点配置参数][parameter] | 是 |
| options| object | [命令位置参数][location]<br>如果不指定该参数，更新操作默认对所有节点生效 | 否 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.9 及以上版本

##示例##

- 更新“在线生效”类型的配置 diaglevel，并指定生效节点为 11820

    ```lang-javascript
    > db.updateConf({diaglevel: 3}, {ServiceName: "11820"})
    ```

- 更新“重启生效”类型的配置 numpreload，并指定生效节点为 11820

    ```lang-javascript
    > db.updateConf({numpreload: 10}, {ServiceName: "11820"})
    ```
    
    如果返回如下信息，需重启节点

    ```lang-javascript
    (shell):1 uncaught exception: -322
    Some configuration changes didn't take effect:
    Config 'numpreload' require(s) restart to take effect.
    ```

- 强制配置非官方参数。
	```lang-javascript
    // 连接协调节点
	> db = new Sdb( "localhost", 11810 )
	> db.updateConf( { aaa: 10 }, { Force: true } )
	```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[parameter]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[location]:manual/Manual/Sequoiadb_Command/location.md