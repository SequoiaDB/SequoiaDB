##名称##

setAttributes - 修改节点的属性

##语法##

**node.setAttributes(\<options\>)**

##类别##

SdbNode

##描述##

该函数用于修改当前节点的属性。

##参数##

options（ *object，必填* ）

通过参数 options 可以修改节点属性：

- Location（ *string* ）：节点位置信息

    该参数取值为空字符串时，表示删除当前节点的位置信息。

    格式：`Location: "GuangZhou"`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

- 将节点 11820 的位置信息修改为"GuangZhou"

    ```lang-javascript
    > var node = db.getRG("group1").getNode("hostname", 11820)
    > node.setAttributes({Location: "GuangZhou"})
    ```

- 删除节点的位置信息

    ```lang-javascript
    > node.setAttributes({Location: ""})
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md