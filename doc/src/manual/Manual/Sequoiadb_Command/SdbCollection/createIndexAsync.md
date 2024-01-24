##名称##

createIndexAsync - 异步创建索引

##语法##

**db.collectionspace.collection.createIndexAsync\(\<name\>, \<indexDef\>, \[indexAttr\], \[option\])**

##类别##

SdbCollection

##描述##

该函数用于为集合异步创建[索引][index]，提高查询速度。

##参数##

| 参数名 | 类型     | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| name   | string | 索引名，同一个集合中的索引名必须唯一 | 是 |
| indexDef | object | 索引键，包含一个或多个指定索引字段与类型的对象<br>类型值 1 表示字段升序，-1 表示字段降序，"text"则表示创建[全文索引][text_index] | 是 |
| indexAttr | object | 索引属性，选项可参考 indexAttr 选项说明| 否 |
| option | object | 控制参数，选项可参考 option 选项说明 | 否 |

indexAttr 选项：

| 属性名          | 类型     | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| Unique          | boolean  | 索引是否唯一 | false  |
| Enforced        | boolean  | 索引是否强制唯一 | false  |
| NotNull         | boolean  | 索引的任意一个字段是否允许为 null 或者不存在 | false  |
| NotArray        | boolean  | 索引的任意一个字段是否允许为数组 | false |

option 选项：

| 属性名          | 类型     | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| SortBufferSize  | number   | 创建索引时使用的排序缓存的大小 | 64MB  |

> **Note:**
>
> * 在唯一索引所指定的索引键字段上，集合中不可存在一条以上的记录完全重复。
> * 索引名限制、索引字段的最大数量、索引键的最大长度请参考[限制][limitation]。
> * 在集合记录数据量较大时（大于1000万条记录）适当增大排序缓存大小可以提高创建索引的速度。
> * 对于全文索引，参数 isUnique、enforced 及 sortBufferSize 无意义。

##返回值##

函数执行成功时，将返回一个 number 类型的对象。通过该对象获取返回的任务 ID。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.6 及以上版本

##示例##

1. 在集合 sample.employee 中创建唯一索引

    ```lang-javascript
    > db.sample.employee.createIndexAsync("ab", {a: 1, b: 1}, {Unique: true})
    1051
    ```

2. 查看相应的任务信息

    ```lang-javascript
    > db.getTask(1051)
    ```


[^_^]:
     本文使用的所有引用和链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[limitation]:manual/Manual/sequoiadb_limitation.md#索引
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[error_guide]:manual/FAQ/faq_sdb.md
[standalone]:manual/Manual/sequoiadb_limitation.md#索引
