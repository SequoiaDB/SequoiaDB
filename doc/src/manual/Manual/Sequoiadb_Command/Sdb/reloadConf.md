##名称##

reloadConf - 重新加载配置文件

##语法##

**db.reloadConf([options])**

##类别##

Sdb

##描述##

该函数用于重新加载配置文件，使配置动态生效。如果配置项不允许动态生效会被忽略。

##参数##

| 参数名 | 类型   | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options | object | [命令位置参数][location] | 否 |

> **Note:**
>
> 用户不指定命令位置参数时，默认对所有节点生效。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

* 对所有节点进行配置重新加载

    ```lang-javascript
    // 连接协调节点
    > db = new Sdb("localhost", 11810)
    > db.reloadConf()
    ```

* 对指定节点 1000 进行配置重加载

    ```lang-javascript
    // 连接协调节点
    > db = new Sdb("localhost", 11810)
    > db.reloadConf({NodeID: 1000})
    ```

[^_^]:
     本文使用的所有引用及链接
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md