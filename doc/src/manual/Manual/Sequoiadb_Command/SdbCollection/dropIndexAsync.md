##名称##

dropIndexAsync - 异步删除索引

##语法##

**db.collectionspace.collection.dropIndexAsync(\<name\>)**

##类别##

SdbCollection

##描述##

该函数用于异步删除集合中指定的[索引][index]。

##参数##

name（ *string，必填* ）

指定删除的索引名

##返回值##

函数执行成功时，将返回一个 number 类型的对象。通过该对象获取返回的任务 ID。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.6 及以上版本

##示例##

1. 删除集合 sample.employee 中名为“ageIndex”的索引

    ```lang-javascript
    > db.sample.employee.dropIndexAsync("ageIndex")
    1051
    ```

2. 查看相应的任务信息

    ```lang-javascript
    > db.getTask(1051)
    ```


[^_^]:
    本文使用的所有引用及链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[error_guide]:manual/FAQ/faq_sdb.md
