##名称##

setLocation - 修改节点的位置信息

##语法##

**doamin.setLocation(\<hostname\>, \<location\>)**

##类别##

SdbDomain

##描述##

该函数用于在当前域中，按主机名批量修改节点的位置信息。

##参数##

- hostname（ *string，必填* ）

    主机名

- location（ *string，必填* ）

    节点位置信息

    - 位置信息的最大长度限制为 256 字节。
    - 取值为空字符串时，表示删除域中节点的位置信息。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setLocation()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺失必填参数 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

- 在域 mydomain 中，将主机 sdbserver1 下节点的位置信息修改为"GuangZhou"

    ```lang-javascript
    > var domain = db.getDomain("mydomain")
    > domain.setLocation("sdbserver1", "GuangZhou")
    ```

- 删除主机 sdbserver1 下节点的位置信息

    ```lang-javascript
    > domain.setLocation("sdbserver1", "")
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[sysnodes]:manual/Manual/Catalog_Table/SYSNODES.md