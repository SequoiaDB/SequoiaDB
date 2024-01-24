##名称##

setPDLevel - 动态设置节点的诊断日志级别

##语法##

**db.setPDLevel(\<level\>, [options])**

##类别##

Sdb

##描述##

该函数用于动态设置节点的诊断日志级别。

##参数##

- level（ *number，必填* ）

    指定日志级别，取值 0~5，分别对应：

    - 0: SEVERE
    - 1: ERROR
    - 2: EVENT
    - 3: WARNING
    - 4: INFO
    - 5: DEBUG

- options（ *object，选填* ）

    指定[命令位置参数][location]

> **Note:**
>
> * 日志级别不正确时默认设置为 WARNING。
> * 无位置参数时，缺省只对本身节点生效。
> * 该设置参数不会被持久化。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

* 设置当前节点的日志级别为 DEBUG

    ```lang-javascript
    // 连接节点
    > db = new Sdb("localhost", 11810)
    > db.setPDLevel(5)
    ```

* 设置节点 1000 的日志级别为 INFO

    ```lang-javascript
    // 连接节点
    > db = new Sdb("localhost", 11810)
    > db.setPDLevel(4, {NodeID: 1000})
    ```

* 设置所有节点的日志级别为 EVENT

    ```lang-javascript
    // 连接节点
    > db = new Sdb("localhost", 11810)
    > db.setPDLevel(3, {Global:true})
    ```

[^_^]:
     本文使用的所有引用及链接
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md