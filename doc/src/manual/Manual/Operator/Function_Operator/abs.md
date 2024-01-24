
##语法##

```lang-json
{ <字段名>: { $abs: 1 } }
```

##说明##

返回数字的绝对值，原始值为数组类型时对每个数组元素执行该操作，其它非数字类型返回 null。

> **Note:**  
>
> { $abs: 1 } 中 1 没有特殊含义，仅作为占位符出现。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": -2 } )
```

* 作为选择符使用，返回字段 a 的绝对值

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$abs": 1 } } )
  {
      "_id": {
        "$oid": "58245d3c2b4c38286d000017"
      },
      "a": 2
  }
  ```

* 与匹配符配合使用，匹配字段 a 的绝对值为 2 的记录

  ```lang-javascript
  > db.sample.employee.find( { "a": { "$abs": 1, "$et": 2 } } )
  {
      "_id": {
        "$oid": "58245d3c2b4c38286d000017"
      },
      "a": -2
  }
  ```
