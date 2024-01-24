
CLCount 对象。

##语法##

**CLCount.hint(\<hint\>)**

**CLCount.valueOf()**

**CLCount.toString()**

##方法##

###hint(\<hint\>)###

统计当前集合符合条件的记录总数，可通过 hint 指定查询使用的索引。

| 参数名 | 参数类型 | 默认值 | 描述                       | 是否必填 |
| ------ | -------- | ------ | -------------------------- | -------- |
| hint   | JSON     | ---    | 指定访问计划，加快查询速度 | 是       |

> **Note：**  
> 1. hint 参数是一个 Json 对象。数据库不关心该对象的字段名，而是通过其字段值来确认需要使用的索引名称。当字段值为 null 时，表示表扫描。使用 hint 参数的格式为：  ```{ "": null }、 { "": "indexname" }、 { "0": "indexname0", "1": "indexname1", "2": "indexname2" }```。  
> 2. v3.0 之前，当使用 hint() 指定索引时，数据库一旦遍历到能够使用的索引（或者表扫描）时，便会停止遍历，进而转向使用该索引（或表扫描）进行数据查找。  
> 3. v3.0 开始，数据库在选择索引时，会基于数据和索引的统计模型进行综合分析，最终会选择一个最恰当的索引使用。所以，从 v3.0 开始，当使用 hint() 指定多个索引时，数据库将能够选择最合适当前查询的索引。

###valueOf()###

获取 CLCount 的原始值。

>**Note:**

>该方法为隐藏方法

###toString()###

把 CLCount 以字符串的形式输出。

>**Note:**

>该方法为隐藏方法

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##示例##

* 指定索引统计当前集合符合条件的记录总数。

   ```lang-javascript
   > var db = new Sdb( "localhost", 11810 )
   > db.sample.employee.find().count().hint( { "": "ageIndex" } )
   50004
   ```

