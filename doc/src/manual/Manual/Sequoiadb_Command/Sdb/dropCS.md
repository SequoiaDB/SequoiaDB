##名称##

dropCS - 删除一个已存在的集合空间。

##语法##

**db.dropCS(\<name\>, [options])**

##类别##

Sdb

##描述##

删除一个已存在的集合空间。

##参数##

* `name` ( *String*， *必填* )

    集合空间名。

* `options` ( *Object*， *选填* )

    在删除集合空间时，可以通过`options`参数设置选项。`options` 选项如下：

    1. `EnsureEmpty` ( *Bool* )：删除集合空间时，是否检查集合空间为空。默认为 false 。其可选取值如下：

        * true：当集合空间下存在集合时，不执行删除操作，返回失败，错误码为 -275 ；当集合空间下不存在集合时，执行删除并返回结果
        * false：不做检查，直接删除集合空间及其包含的相关集合

        格式：`EnsureEmpty:true|false`

    2. `SkipRecycleBin` ( *Bool* )：删除集合空间时，是否禁用[回收站][recycle_bin]机制，默认是 false。其可选取值如下：

        * true：删除集合空间时，将不生成对应的回收站项目
        * false：根据字段 [Enable][getDetail] 的值决定是否启用回收站机制

        格式：`SkipRecycleBin:true|false`

##返回值##

成功：指定的集合空间被删除。

失败：抛出异常。

##错误##

`dropCS()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -275 | SDB_DMS_CS_NOT_EMPTY | 集合空间中存在集合。| 检查是否开启了 `EnsureEmpty` 选项。|
| -386 | SDB_RECYCLE_FULL | 回收站已满。 | 检查回收站是否已满，并通过 [dropItem()][dropItem] 或 [dropAll()][dropAll] 手动清理回收站。 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，
或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。
可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v1.0及以上版本。

##例子##

1. 删除名为 sample 的集合空间，假定 sample 已存在

    ```lang-javascript
    > db.dropCS("sample")
    ```

[^_^]:
     本文使用的所有链接及引用
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[recycle_bin]:manual/Distributed_Engine/Maintainance/recycle_bin.md
[dropItem]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md
[dropAll]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md