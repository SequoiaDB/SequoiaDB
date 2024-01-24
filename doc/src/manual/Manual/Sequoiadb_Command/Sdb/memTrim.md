##名称##

memTrim - 对指定节点或全部节点进行空闲内存回收

##语法##

**db.memTrim([mask], [options])**

##类别##

Sdb

##描述##

该函数用于对指定节点或全部节点进行空闲内存回收，在回收 "OSS" 内存时如果内存碎片非常多，可能会发生节点进程卡顿，阻塞其它操作。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| mask | string | OSS, POOL, TC，ALL，可以使用 '\|' 进行组合，当不指定时默认为 OSS | 否 |
| options| object | [命令位置参数][location]<br>如果不指定该参数，更新操作默认对所有节点生效 | 否 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v7.0,v5.6.2 及以上版本

##示例##

- 对所有节点进行"OSS"空闲内存回收

    ```lang-javascript
    > db.memTrim()
    ```

- 对所有数据节点进行"ALL"内存回收

    ```lang-javascript
    > db.memTrim("ALL", {Role: "data"})
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[location]:manual/Manual/Sequoiadb_Command/location.md