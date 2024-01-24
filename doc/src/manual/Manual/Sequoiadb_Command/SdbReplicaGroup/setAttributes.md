##名称##

setAttributes - 修改复制组的属性

##语法##

**rg.setAttributes(\<options\>)**

##类别##

SdbReplicaGroup 

##描述##

该函数用于修改当前复制组的属性。

##参数##

options（ *object，必填* ）

通过参数 options 可以修改复制组属性：

- ActiveLocation（ *string* ）：ActiveLocation 对应的位置集

    - 指定的位置集需存在于当前复制组中。
    - 取值为空字符串时，表示删除当前复制组的 ActiveLocation。

    格式：`ActiveLocation: "GuangZhou"`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

- 将位置集"GuangZhou"设置为复制组 group1 的 ActiveLocation

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setAttributes({ActiveLocation: "GuangZhou"})
    ```

- 删除复制组 group1 的 ActiveLocation

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setAttributes({ActiveLocation: ""})
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md