##名称##

stopMaintenanceMode - 在当前复制组中停止 Maintenance 模式

##语法##

**rg.stopMaintenanceMode(\[options\])**

##类别##

SdbReplicaGroup 

##描述##

该函数用于在当前复制组中停止 Maintenance 模式。

##参数##

options（ *object，选填* ）

通过参数 options 可以指定停止 Maintenance 模式的参数：

- NodeName（ *string* ）：停止节点的 Maintenance 模式

    指定的节点需存在于当前复制组中。

    格式：`NodeName: "sdbserver:11820"`

- Location（ *string* ）：停止位置集所有节点的 Maintenance 模式

    - 指定的位置集需存在于当前复制组中。
    - 该参数仅在未指定 NodeName 时生效。

    格式：`Location: "GuangZhou"`

> **Note:**
>
> 如不指定 options 参数，或 options 参数为空 {}，表示停止复制组中所有节点的 Maintenance 模式。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8 及以上版本

##示例##

在复制组 group1 中停止所有节点的 Maintenance 模式

```lang-javascript
> var rg = db.getRG("group1")
> rg.stopMaintenanceMode()
```

在复制组 group1 中停止位置集为 GuangZhou 节点的 Maintenance 模式

```lang-javascript
> var rg = db.getRG("group1")
> rg.stopMaintenanceMode({Location: GuangZhou})
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
