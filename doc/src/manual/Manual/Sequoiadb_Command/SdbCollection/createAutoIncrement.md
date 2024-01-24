##名称##

createAutoIncrement - 创建自增字段

##语法##

**db.collectionspace.collection.createAutoIncrement(\<option|options\>)**

##类别##

SdbCollection

##描述##

该函数用于在指定集合中创建一个或多个自增字段。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| option&#124;options | object/array | 自增字段参数。option 是一个字段的参数，options 是多个字段的参数。 | 是 |

option中的具体参数请参见[自增字段属性](manual/Distributed_Engine/Architecture/Data_Model/sequence.md)

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 创建一个自增字段，其值总是由服务端生成，忽略客户端设置。

    ```lang-javascript
    > db.sample.employee.createAutoIncrement( { Field: "studentID", Generated: "always" } )
    ```

* 创建两个自增字段

    ```lang-javascript
    > db.sample.employee.createAutoIncrement( [ { Field: "comID" }, { Field: "innerID" } ] )
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
