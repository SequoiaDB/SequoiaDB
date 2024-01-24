##名称##

startMaintenanceMode - 在复制组中开启 Maintenance 模式

##语法##

**rg.startMaintenanceMode(\<options\>)**

##类别##

SdbReplicaGroup 

##描述##

该函数用于在复制组中对指定范围的节点开启 Maintenance 模式，处于该模式的节点不参与复制组选举与同步一致性 replSize 计算。

> **Note:**
>
> 如复制组中全部节点都处于 Maintenance 模式，该复制组不可用，且无主节点。

##参数##

options（ *object，必填* ）

通过参数 options 可以指定 Maintenance 模式的参数：

- NodeName（ *string* ）：Maintenance 模式生效的节点

    指定的节点需存在于当前复制组中。

    格式：`NodeName: "sdbserver:11820"`

    > **Note:**  
    >
    > 复制组主节点无法开启 Maintenance 模式。

- Location（ *string* ）：Maintenance 模式生效的位置集

    - 指定的位置集需存在于当前复制组中。
    - 该参数仅在未指定 NodeName 时生效。

    格式：`Location: "GuangZhou"`

- MinKeepTime（ *number* ）：Maintenance 模式的最低运行窗口时间，取值范围为(0, 10080]，单位为分钟

    格式：`MinKeepTime: 100`

- MaxKeepTime（ *number* ）：Maintenance 模式的最高运行窗口时间，取值范围为(0, 10080]，单位为分钟

    格式：`MaxKeepTime: 1000`

    > **Note:**
    >
    > 对于 MinKeepTime 和 MaxKeepTime：
    > - MinKeepTime 的取值应小于 MaxKeepTime。
    > - 成功开启 Maintenance 模式后，在未达到 MinKeepTime 指定的时间前，节点会一直保持 Maintenance 模式；在 MinKeepTime-MaxKeepTime 时间段内，如果节点状态正常，会自动解除 Maintenance 模式；超过 MaxKeepTime 指定的时间后，节点将强制解除 Maintenance 模式。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`startMaintenanceMode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺失必填参数 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8 及以上版本

##示例##

在复制组 group1 中开启 Maintenance 模式

```lang-javascript
> var rg = db.getRG("group1")
> rg.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
```

查看复制组 group1 中开启的 Maintenance 模式

```lang-javascript
> db.list(SDB_LIST_GROUPMODES, {GroupID: 1001})
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
