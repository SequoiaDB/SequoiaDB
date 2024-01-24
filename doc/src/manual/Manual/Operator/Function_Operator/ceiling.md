
##语法##

```lang-json
{ <字段名>: { $ceiling: <值> } }
```

##说明##

返回大于目标字段值的最小整数值，原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null 。

> **Note:**  
> { $ceiling: 1 }中1没有特殊含义，仅作为占位符出现。

##示例##

在集合 sample.employee 插入1条记录：

```lang-javascript 
> db.sample.employee.insert( { "a": 29.2 } )
```

* 作为选择符使用，返回字段 a 向上取整的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$ceiling": 1 } } )
  {
      "_id": {
        "$oid": "5825395a2b4c38286d00001a"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，匹配字段 a 向上取整之后值为 30 的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$ceiling": 1, "$et": 30 } } )
  {
      "_id": {
        "$oid": "5825395a2b4c38286d00001a"
      },
      "a": 29.2
  }
  Return 1 row(s).
  ```
