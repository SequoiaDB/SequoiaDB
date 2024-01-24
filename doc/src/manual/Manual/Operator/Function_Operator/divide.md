
##语法##

```lang-json
{ <字段名>: { $divide: <值> } }
```

##说明##

返回字段值除以某个数值的结果，原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": 300 } )
``` 

* 作为选择符使用，返回字段 a 除以 10 的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$divide": 10 } } )
  {
      "_id": {
        "$oid": "582558152b4c38286d00001b"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

  > **Note:**
  >
  > 除数不能为0。

* 与匹配符配合使用，匹配字段 a 除以 10 的商为 30 的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$divide": 10, "$et": 30 } } )
  {
      "_id": {
        "$oid": "582558152b4c38286d00001b"
      },
      "a": 300
  }
  Return 1 row(s).
  ```
