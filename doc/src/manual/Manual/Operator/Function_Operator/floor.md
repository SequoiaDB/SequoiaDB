
##语法##

```lang-json
{ <字段名>: { $floor: 1 } }
```

##说明##

返回小于目标字段值的最大整数值，原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": 30.8 } )
```

* 作为选择符使用，返回字段 a 向下取整的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$floor": 1 } } )
  {
      "_id": {
        "$oid": "58255d572b4c38286d00001c"
      },
      "a": 30
  }
  Return 1 row(s).
  ```

  > **Note:** 
  > 
  > { $floor: 1 } 中1没有特殊含义，仅作为占位符出现。

* 与匹配符配合使用，匹配字段 a 向下取整之后值为 30 的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$floor": 1, "$et": 30 } } )
  {
      "_id": {
        "$oid": "58255d572b4c38286d00001c"
      },
      "a": 30.8
  }
  Return 1 row(s).
  ```
