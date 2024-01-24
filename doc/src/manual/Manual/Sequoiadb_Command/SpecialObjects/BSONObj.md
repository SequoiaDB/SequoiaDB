
BSON 对象。

数据类型的介绍可参考[数据类型](manual/Distributed_Engine/Architecture/Data_Model/data_type.md)。

##语法##

**BSONObj(\<json\>) / new BSONObj(\<json\>)**

**BSONObj.toJson()**

**BSONObj.toObj()**

**BSONObj.toString()**

##方法##

###BSONObj(\<json\>) / new BSONObj(\<json\>)###

创建 BSONObj 对象

| 参数名 | 参数类型 | 默认值 | 描述      | 是否必填 |
| ------ | -------- | ------ | --------- | -------- |
| json   | JSON     | ---    | json 数据 | 是       |

###toJson()###

把 BSONObj 转换成 JSON 字符串。

###toObj()###

把 BSONObj 转换成 JSON 对象。

###toString()###

把 BSONObj 以字符串的形式输出。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。


##示例##

* 获取 BSON 对象。

   ```lang-javascript
   > var db = new Sdb( "localhost", 11810 )
   > var bsonObj = db.sample.employee.find().current()
   ```

* 把 BSONObj 转换成 JSON 字符串。

   ```lang-javascript
   > bsonObj.toJson() 
   { "_id": { "$oid": "5d240ab1117b8a87cbfd10eb" }, "age": 17, "name": "tom" }
   ```

* 把 BSONObj 转换成 JSON 对象。

   ```lang-javascript
   > var obj = bsonObj.toObj()
   > obj.age
   17
   > obj.name
   tom 
   ```

* 把 BSONObj 以字符串的形式输出。

   ```lang-javascript
   > bsonObj.toString()
   {
      "_id": {
        "$oid": "5d240ab1117b8a87cbfd10eb"
      },
      "age": 17,
      "name": "tom"
   }
   ```

* BSONObj 对象也可以自己创建。

   ```lang-javascript
   > var newBSONObj = new BSONObj( { name: "fang" } )
   > newBSONObj
   {
      "name": "fang"
   }
   ```
