
##语法##

```lang-json
{ <字段名>: { $add: <值> } }
```

##说明##

返回字段值加上某个数值的结果，原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": 20 } )
```

* 作为选择符使用，字段 a 加上 10 的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$add": 10 } } )
  {
      "_id": {
        "$oid": "58251ca32b4c38286d000018"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，字段 a 加 10 的和为 30 的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$add": 10, "$et": 30 } } )
  {
      "_id": {
        "$oid": "58251ca32b4c38286d000018"
      },
      "a": 20
  }
  Return 1 row(s).
  ```