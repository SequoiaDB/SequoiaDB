
##语法##

```lang-json
{ <字段名>: { $mod: [ <value1>, <value2> ] }, ... }
```

##描述##

$mod 是取模匹配符，返回指定“\<字段名\>”的值对 “\<value1\>”取模的值等于“\<value2\>”的记录。

> **Note:**  
>
> - 不能对 0 取模；  
> - 由于操作系统提供的浮点数（IEEE754 浮点数标准）的特性，浮点数的模运算结果是不准确的，超出 15 位有效数字的浮点数的结果甚至会严重偏离准确值，所以不建议对浮点数进行模运算，尤其是对浮点数模运算的结果进行“==”和“!=”的判断。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert( { "age": 3 } )
```

查询集合 sample.employee 中 age 字段值对 5 取模后的值等于 3 的记录

```lang-javascript
> db.sample.employee.find( { "age": { "$mod": [ 5, 3 ] } } )
{
    "_id": {
      "$oid": "5824335d2b4c38286d00000f"
    },
    "age": 3
}
```


