##名称##

startCriticalMode - 在复制组中开启 Critical 模式

##语法##

**rg.startCriticalMode(\<options\>)**

##类别##

SdbReplicaGroup 

##描述##

该函数用于在一个不具备选举条件的复制组中开启 Critical 模式，并在指定的节点范围内选举出主节点。

##参数##

options（ *object，必填* ）

通过参数 options 可以指定 Critical 模式的参数：

- NodeName（ *string* ）：Critical 模式生效的节点

    指定的节点需存在于当前复制组中。

    格式：`NodeName: "sdbserver:11820"`

- Location（ *string* ）：Critical 模式生效的位置集

    - 指定的位置集需存在于当前复制组中。
    - 该参数仅在未指定 NodeName 时生效。

    格式：`Location: "GuangZhou"`

    > **Note:**  
    >
    > 在编目复制组开启 Critical 模式时，生效节点必须包含当前复制组的主节点。

- MinKeepTime（ *number* ）：Critical 模式的最低运行窗口时间，取值范围为(0, 10080]，单位为分钟

    格式：`MinKeepTime: 100`

- MaxKeepTime（ *number* ）：Critical 模式的最高运行窗口时间，取值范围为(0, 10080]，单位为分钟

    格式：`MaxKeepTime: 1000`

    > **Note:**
    >
    > 对于 MinKeepTime 和 MaxKeepTime：
    > - MinKeepTime 的取值应小于 MaxKeepTime。
    > - 成功开启 Critical 模式后，在未达到 MinKeepTime 指定的时间前，复制组会一直保持 Critical 模式；在 MinKeepTime-MaxKeepTime 时间段内，如果复制组内大多数节点正常，会自动解除 Critical 模式；超过 MaxKeepTime 指定的时间后，复制组将强制解除 Critical 模式。

- Enforced（ *boolean* ）：是否强制开启 Critical 模式，默认值为 false

    该参数选填，取值如下：

    - true：复制组将在 Critical 模式的生效节点范围内强制生成主节点。如果在生效节点范围外存在 LSN 更高的节点，强制执行会导致数据回滚。
    - false：在开启过程中，如果在生效节点范围外检测到 LSN 更高的节点，操作将报错。

    格式：`Enforced: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`startCriticalMode()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -13 | SDB_TIMEOUT | 开启 Critical 模式超时 | 检查生效节点范围外是否存在 LSN 更高的节点 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺失必填参数 |
| -334 | SDB_OPERATION_CONFLICT | 参数范围错误 | 检查编目的主节点是否在 Critical 模式的生效节点范围内 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

在复制组 group1 中开启 Critical 模式

```lang-javascript
> var rg = db.getRG("group1")
> rg.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
```

查看复制组 group1 中开启的 Critical 模式

```lang-javascript
> db.list(SDB_LIST_GROUPMODES, {GroupID: 1001})
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
