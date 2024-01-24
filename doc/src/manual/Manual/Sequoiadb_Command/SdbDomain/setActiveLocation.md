##名称##

setActiveLocation - 设置域中所有复制组的 ActiveLocation

##语法##

**domain.setActiveLocation(\<location\>)**

##类别##

SdbDomain

##描述##

该函数用于在当前域中，同时设置所有复制组的 ActiveLocation。

##参数##

location（ *string，必填* ）

位置集名称

- 指定的位置集需存在于域包含的所有复制组中。
- 取值为空字符串时，表示删除域中所有复制组的 ActiveLocation。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setActiveLocation()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺失必填参数 |
| -264 | SDB_COORD_NOT_ALL_DONE | 部分复制组中不存在指定的位置集 | 检查设置失败的复制组中是否存在该位置集 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

- 域 mydomain 包含复制组 group1 和 group2，将域中所有复制组的 ActiveLocation 设置为"GuangZhou"

    ```lang-javascript
    > var domain = db.getDomain("mydomain")
    > domain.setActiveLocation("GuangZhou")
    ```

    查看是否设置成功

    ```lang-javascript
    > db.list(SDB_LIST_GROUPS, {}, {ActiveLocation: "", GroupName: ""})
    ...
    {
      "ActiveLocation": "GuangZhou",
      "GroupName": "group1"
    }
    {
      "ActiveLocation": "GuangZhou",
      "GroupName": "group2"
    }
    ...
    ```

- 删除域中所有复制组的 ActiveLocation

    ```lang-javascript
    > domain.setActiveLocation("")
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md