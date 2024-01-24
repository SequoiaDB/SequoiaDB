##名称##

dropCL - 删除当前集合空间下指定的集合

##语法##

**db.collectionspace.dropCL(\<name\>, [options])**

##类别##

SdbCS

##描述##

该函数用于删除当前集合空间下指定的集合。

##参数##

* name（ *string，必填* ）

    集合名

* options（ *object，选填* ）

    通过 options 可以设置其他选填参数：

    - SkipRecycleBin（ *boolean* ）：是否禁用[回收站][recycle_bin]机制，默认是 false，表示根据字段 [Enable][getDetail] 的值决定是否启用回收站机制

        该参数取值为 true，表示删除集合时将不生成对应的回收站项目。

        格式：`SkipRecycleBin: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`dropCL()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | 集合不存在| 检查集合是否存在|
| -386 | SDB_RECYCLE_FULL |  回收站已满 | 检查回收站是否已满，并通过 [dropItem()][dropItem] 或 [dropAll()][dropAll] 手动清理回收站  |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.0 及以上版本

##示例##

删除集合空间 sample 下的集合 employee

```lang-javascript
> db.sample.dropCL("employee")
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[recycle_bin]:manual/Distributed_Engine/Maintainance/recycle_bin.md
[dropItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md
[dropAll]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md