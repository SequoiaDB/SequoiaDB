##名称##

hint - 按指定的索引遍历结果集

##语法##

**query.hint( \<hint\> )**

##类别##

SdbQuery

##描述##

按指定的索引遍历结果集。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述                       | 是否必填 |
| ------ | -------- | ------ | -------------------------- | -------- |
| hint   | JSON     | ---    | 指定访问计划，加快查询速度 | 是       |

> **Note：**  
> 1. hint 参数是一个 Json 对象。数据库不关心该对象的字段名，而是通过其字段值来确认需要使用的索引名称。当字段值为 null 时，表示表扫描。使用 hint 参数的格式为：  ```{ "": null }、 { "": "indexname" }、 { "0": "indexname0", "1": "indexname1", "2": "indexname2" }```。  
> 2. v3.0之前，当使用hint()指定索引时，数据库一旦遍历到能够使用的索引（或者表扫描）时，便会停止遍历，进而转向使用该索引（或表扫描）进行数据查找。  
> 3. v3.0开始，数据库在选择索引时，会基于数据和索引的统计模型进行综合分析，最终会选择一个最恰当的索引使用。所以，从v3.0开始，当使用hint()指定多个索引时，数据库将能够选择最合适当前查询的索引。


##返回值##

返回查询结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

* 强制要求查询走表扫描。

    ```lang-javascript
    > db.sample.employee.find( {age: 100 } ).hint( { "": null } )
    ```

* 使用索引 ageIndex 遍历集合 employee 下存在 age 字段的记录，并返回。

    ```lang-javascript
    > db.sample.employee.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
    {
       "_id": {
         "$oid": "5cf8aef75e72aea111e82b38"
       },
       "name": "tom",
       "age": 20
     }
     {
       "_id": {
         "$oid": "5cf8aefe5e72aea111e82b39"
       },
       "name": "ben",
       "age": 21
     }
     {
       "_id": {
         "$oid": "5cf8af065e72aea111e82b3a"
       },
       "name": "alice",
       "age": 19
    }
    ```

* 提供若干索引，供数据库选择。数据库将基于数据和索引统计，选择最优的索引使用。

    ```lang-javascript
    > db.sample.employee.find( { age: 100 } ).hint( { "1": "aIndex", "2": "bIndex", "3":"cIndex" } )
    ```