##名称##

createIdIndex - 创建 $id 索引

##语法##

**db.collectionspace.collection.createIdIndex\(\[options\])**

##类别##

SdbCollection

##描述##

该函数用于创建 $id 索引，在 SequoiaDB 中创建集合时可以根据需要将 AutoIndexId 置为 false。这样集合将不会创建默认的“$id”索引，同时数据的更新、删除操作将被禁止。本方法可以恢复“$id”索引，同时开放更新和删除功能。

##参数##

| 参数名  | 参数类型 | 默认值 | 描述   | 是否必填 |
| ------- | -------- | ------ | ------ | -------- |
| options | JSON     | ---    | 可选项 | 否       |

options 参数详细说明如下：

| 属性名 | 值类型 | 默认值 | 描述 |
| ------ | ------ | ------ | ---- |
| SortBufferSize | int | 64 | 创建索引时使用的排序缓存的大小（单位：MB），取值为 0 时表示不使用排序缓存 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

| 错误码  | 可能的原因       |  解决方法  |
| ------- | ---------------- | ---------- |
| -247    | $id 索引已经存在 |    -       |
| -291    | 存在一个相同定义的索引 |   删除定义冲突的索引 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 使用默认参数构建 $id 索引:

    ```lang-javascript
    > db.sample.employee.createIdIndex()
    ```

* 构建 $id 索引时指定排序缓存大小:

    ```lang-javascript
    > db.sample.employee.createIdIndex( { SortBufferSize: 128 } )
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md