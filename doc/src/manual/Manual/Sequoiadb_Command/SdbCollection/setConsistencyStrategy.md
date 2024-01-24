##名称##

setConsistencyStrategy - 修改集合的同步一致性策略

##语法##

**db.collectionspace.collection.setConsistencyStrategy(\<strategy\>)**

##类别##

SdbCollection

##描述##

该函数用于修改集合的[同步一致性][consistency_strategy]策略。

##参数##

strategy（ *number，必填* ）

该参数用于修改指定集合的同步一致性策略

取值如下：

- 1：节点优先策略
- 2：位置多数派优先策略
- 3：主位置多数派优先策略

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setConsistencyStrategy()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误或取值不存在 | 检查当前参数类型和取值 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

1. 将集合 sample.employee 的同步一致性策略修改为节点优先策略

    ```lang-javascript
    > db.sample.employee.setConsistencyStrategy(1)
    ```

2. 查看当前集合的同步一致性策略

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CATALOG, {Name: "sample.employee"}, {"ConsistencyStrategy": null})
    {
      "ConsistencyStrategy": 1
    }
    ```

[^_^]:
    本文使用的所有引用和链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[consistency_strategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md