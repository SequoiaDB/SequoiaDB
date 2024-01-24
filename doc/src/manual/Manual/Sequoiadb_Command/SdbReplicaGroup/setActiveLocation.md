##名称##

setActiveLocation - 设置复制组的 ActivedLocation

##语法##

**rg.setActiveLocation(\<location\>)**

##类别##

SdbReplicaGroup

##描述##

该函数用于在当前复制组中，将指定的位置集设置为 ActiveLocation。

##参数##

location（ *string，必填* ）

位置集名称

- 指定的位置集需存在于当前复制组中。
- 取值为空字符串时，表示删除当前复制组的 ActiveLocation。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setActiveLocation()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺失必填参数 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

- 将位置集"GuangZhou"设置为复制组 group1 的 ActiveLocation

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setActiveLocation("GuangZhou")
    ```

- 删除复制组 group1 的 ActiveLocation

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setActiveLocation("")
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md