##名称##

remove - 删除集合中的记录

##语法##

**db.collectionspace.collection.remove\(\[cond\], \[hint\], \[options\]\)**

##类别##

SdbCollection

##描述##

该函数用于删除集合中的记录。

##参数##

| 参数名 | 类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| cond   | object| 选择条件，为空时，删除所有记录，不为空时，删除符合条件的记录 | 否 |
| hint   | object| 指定访问计划 | 否 |
| options| object| 可选项，详见 options 选项说明| 否 |

options 选项：

| 参数名          | 类型 | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| JustOne         | boolean     | 为 true 时，将只更新一条符合条件的记录<br>为 false 时，将会更新所有符合条件的记录| false  |

> **Note:**
>
> - 参数 cond 和 hint 的用法与 [find()][find] 的相同。
> - JustOne 为 true 时，只能在单个分区、单个子表上执行。

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取成功删除的相关信息，字段说明如下：

| 字段名 | 类型 | 描述 |
|--------|------|------|
| DeletedNum | int64 | 成功删除的记录数 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`remove()` 函数常见异常如下：
  
| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -348     | SDB_COORD_DELETE_MULTI_NODES|参数 JustOne 为 true 时，跨多个分区或多个子表删除记录 | 修改匹配条件或不使用参数 JustOne |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

- 删除集合所有记录

    ```lang-javascript
    > db.sample.employee.remove()
    ```

- 按访问计划删除匹配 cond 条件的记录，如下操作按照索引名为"myIndex"的索引遍历集合中的记录，在遍历得到的记录中删除符合条件 age 字段值大于等于 20 的记录

    ```lang-javascript
    > db.sample.employee.remove({age: {$gte: 20}}, {"": "myIndex"})
    ```

[^_^]:
    本文使用的所有引用及链接
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
