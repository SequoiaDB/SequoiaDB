
##语法##

```lang-json
{ <字段名>: { $upper: 1 } }
```

##说明##

返回字符串中字符改变为大写的结果，原始值为数组类型时对每个数组元素执行该操作，非字符串类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": "abc" } )
```

* 作为选择符使用，返回字段 a 转换为大写的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$upper": 1 } } )
  {
      "_id": {
        "$oid": "58257f33ec5c9b3b7e000005"
      },
      "a": "ABC"
  }
  Return 1 row(s).
  ```

  > **Note:**  
  >
  > { $upper: 1 } 中 1 没有特殊含义，仅作为占位符出现。

* 与匹配符配合使用，匹配字段 a 转换为大写之后值为"ABC"的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$upper": 1, "$et": "ABC" } } )
  {
      "_id": {
        "$oid": "58257f33ec5c9b3b7e000005"
      },
      "a": "abc"
  }
  Return 1 row(s).
  ```
