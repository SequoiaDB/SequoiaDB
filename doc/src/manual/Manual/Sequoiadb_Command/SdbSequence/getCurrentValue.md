##名称##

getCurrentValue - 获取序列的当前值

##语法##

**sequence.getCurrentValue\(\)**

##类别##

SdbSequence

##描述##

当用户需要知道序列进度时，可以使用该函数用获取序列的当前值。

##参数##

无

##返回值##

函数执行成功时，将以数值形式返回序列的当前值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getCurrentValue()` 函数常见异常如下：

| 错误码 | 错误类型                | 可能发生的原因 | 解决办法 |
| ------ | ----------------------- | -------------- | -------- |
| -362   | SDB_SEQUENCE_NEVER_USED | 序列未经使用   | 检查序列使用情况 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

* 如果获取未经使用的序列当前值，则获取失败

    ```lang-javascript
    > var sequence = db.createSequence("IDSequence")
    > sequence.getCurrentValue()
    (shell):1 uncaught exception: -362
    Sequence has never been used
    ```

* 如果获取已使用的序列当前值，则获取成功

    ```lang-javascript
    > sequence.getCurrentValue()
    1
    ```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
