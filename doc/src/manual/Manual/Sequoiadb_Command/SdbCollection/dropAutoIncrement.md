##名称##

dropAutoIncrement - 删除自增字段

##语法##

**db.collectionspace.collection.dropAutoIncrement\(\<name|names\>\)**

##类别##

SdbCollection

##描述##

该函数用于在指定集合中删除一个或多个自增字段。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| name&#124;names | String | 自增字段名。name 是一个字段的名称，names 是多个字段的名称。 | 是 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.0 及以上版本

##示例##

* 删除一个自增字段

    ```lang-javascript
    > db.sample.employee.dropAutoIncrement( "studentID" )
    ```

* 删除多个自增字段

    ```lang-javascript
    > db.sample.employee.dropAutoIncrement( [ "comID", "innerID" ] )
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md

