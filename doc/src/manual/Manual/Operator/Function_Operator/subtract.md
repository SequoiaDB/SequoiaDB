
##语法##

```lang-json
{ <字段名>: { $subtract: <值> } }
```

##说明##

返回字段值减去某个数值的结果，原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": 40 } )
```

* 作为选择符使用，返回字段 a 减去 10 的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$subtract": 10 } } )
  {
      "_id": {
        "$oid": "58257c0dec5c9b3b7e000003"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，匹配字段 a 减去 10 之差为 30 的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$subtract": 10, "$et": 30 } } )
  {
      "_id": {
        "$oid": "58257c0dec5c9b3b7e000003"
      },
      "a": 40
  }
  Return 1 row(s).
  ```