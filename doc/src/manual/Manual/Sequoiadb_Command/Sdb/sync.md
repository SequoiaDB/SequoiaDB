##名称##

sync - 持久化数据和日志到磁盘

##语法##

**db.sync([options])**

##类别##

Sdb

##描述##

该函数用于持久化数据和日志到磁盘。

##参数##

options（ *object，选填* ）

通过 options 参数可以设置深度刷盘、阻塞模式、指定集合空间以及[命令位置参数][location]：

- Deep（ *number* ）：是否开启深度刷盘，缺省为 1

    取值如下：

    - 0：表示不开启
    - 1：表示开启
    - -1：表示采用服务器端默认配置

    Deep 取值兼容 boolean 类型。

    格式：`Deep: 1`

- Block（ *boolean* ）：持久化期间是否阻塞所有的变更操作，缺省为 false

    格式：`Block: false`

- CollectionSpace（ *string* ）：指定持久化的集合空间名称

    如果指定该参数，则只会持久化该集合空间，否则会持久化所有的集合空间和日志文件。

    格式：`CollectionSpace: "sample"`

-  Location Elements：命令位置参数项
  
    格式：`GroupName: "db1"`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`sync()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决方法 |
| ------ | ------ | --- | ------ |
| -67    | SDB_BACKUP_HAS_ALREADY_START | 与离线备份任务冲突 | 关闭'阻塞模式'并重试 |
|-149    | SDB_REBUILD_HAS_ALREADY_START | 与本地重建任务冲突 | 关闭'阻塞模式'并重试 |
| -148   | SDB_DMS_STATE_NOT_COMPATIBLE | 与其它阻塞操作冲突（如其它 sync 操作） | 关闭'阻塞模式'并重试 |
| -34    | SDB_DMS_CS_NOTEXIST | 指定集合空间不存在 | 确认集合空间是否存在 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

- 对全系统所有集合空间和日志进行深度持久化

    ```lang-javascript
    > db.sync()
    ```

- 对指定集合空间 sample 进行深度持久化

    ```lang-javascript
    > db.sync({CollectionSpace: "sample"})
    ```

- 对指定数据组 group1 进行深度加阻塞的持久化

    ```lang-javascript
    > db.sync({GroupName: "group1", Block: true})
    ```

[^_^]:
     本文使用的所有引用及链接
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md