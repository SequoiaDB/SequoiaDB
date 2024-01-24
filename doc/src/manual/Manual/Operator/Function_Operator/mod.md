
##语法##

```lang-json
{ <字段名>: { $mod: <值> } }
```

##说明##

返回取模的结果，原始值为数组类型时对每个数组元素执行该操作，非数字类型返回 null 。

> **Note：**  
>
> - 不能对 0 取模；
> - 由于操作系统提供的浮点数（IEEE754 浮点数标准）的特性，浮点数的模运算结果是不准确的，超出 15 位有效数字的浮点数的结果甚至会严重偏离准确值，所以不建议对浮点数进行模运算，尤其是对浮点数模运算的结果进行“==”和“!=”的判断。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": 13 } )
```

* 作为选择符使用，返回字段 a 对 10 取模的结果

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$mod": 10 } } )
  {
      "_id": {
        "$oid": "582568c42b4c38286d00001f"
      },
      "a": 3
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，匹配字段 a 对 10 取模之后值为 3 的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$mod": 10, "$et": 3 } } )
  {
      "_id": {
        "$oid": "582568c42b4c38286d00001f"
      },
      "a": 13
  }
  Return 1 row(s).
  ```

