
##语法##

```lang-json
{ <字段名>: { $rtrim: 1 } }
```

##说明##

去掉字符串右侧开头的空格(回车符 '\\r'、换行符 '\\n' 以及制表符 '\\t' )。原始值为数组类型时对每个数组元素执行该操作，非字符串类型返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": "abc " } )
```


* 作为选择符使用，返回字段 a 去掉右侧开头空格之后的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$rtrim": 1 } } )
  {
      "_id": {
        "$oid": "58256b1a2b4c38286d000021"
      },
      "a": "abc"
  }
  Return 1 row(s).
  ```

  > **Note:**  
  >
  > { $rtrim: 1 } 中的1作为占位符出现。  
  > 原值右边的空格已经去掉。

* 与匹配符配合使用，匹配字段 a 去掉右侧开头空格之后的值为"abc"的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$rtrim": 1, "$et": "abc" } } )
  {
      "_id": {
        "$oid": "58256b1a2b4c38286d000021"
      },
      "a": "abc "
  }
  Return 1 row(s).
  ```
  