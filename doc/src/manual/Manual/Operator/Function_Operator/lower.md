
##语法##

```lang-json
{ <字段名>: { $lower: 1 } }
```

##说明##

返回字符串中字符改变为小写的结果，原始值为数组类型时对每个数组元素执行该操作，非字符串类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": "Abc" } )
```

* 作为选择符使用，返回字段 a 转换为小写的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$lower": 1 } } )
  {
      "_id": {
        "$oid": "58255e6c2b4c38286d00001d"
      },
      "a": "abc"
  }
  Return 1 row(s).
  ```

  > **Note:**  
  >
  > { $lower: 1 } 中的 1 作为占位符出现。

* 与匹配符配合使用，匹配字段 a 转换为小写之后的值为"abc"的记录：  

  ```lang-javascript
  > db.sample.employee.find( { "a": { "$lower": 1, "$et": "abc" } } )
  {
      "_id": {
        "$oid": "58255e6c2b4c38286d00001d"
      },
      "a": "Abc"
  }
  Return 1 row(s).
  ```
