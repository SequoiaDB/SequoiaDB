
SequoiaDB 巨杉数据库内置 SQL 支持算术表达式和比较表达式。

## 算术表达式 ##

### 格式 ###

***\<field1_name\> \<[mathematical_operator](manual/Manual/SQL_Grammar/operator.md#算术运算符)\> \<value\>***

>**Note:** 

> 支持复合运算，如 select \( a + 1 \) * \( a - 1 \), b + 1 from sample.employee

### 示例 ###

age 字段值+1

```lang-javascript
age + 1
```

取集合中age字段，age 值+1

```lang-javascript
> db.exec( "select age + 1 from sample.employee" )
{
  "age": 12
}
```

## 比较表达式 ##

### 格式 ###

***\<field1_name\> \<[comparison_operator][operator]\> \<value | field2_name\>***

***\<field1_name\> \<[comparison_operator][operator]\> \<value\> \<[logical_operator][operator1]\> \<field1_name\> \<comparison_operator\> \<value\>***

### 示例 ###

*  age 字段大于 11

   ```lang-javascript
   age > 11
   ```

   查询匹配 age 大于 11 的记录

   ```lang-javascript
    > db.exec( "select * from sample.employee where age > 11" )
    {
      "_id": {
      "$oid": "5aa3330fdc5673331f000000"
      },
      "name": "Lucy",
      "age": 12
    }
   ```
*  age 字段大于等于 11，且 name 为“Harry”

   ```lang-javascript
   age >= 11 AND name = 'Harry'
   ```
  
   查询匹配 age 大于等于 11，且 name 为“Harry”的记录
  
   ```lang-javascript
    > db.exec( "select * from sample.employee where age >= 11 AND name = 'Harry'" )
    {
      "_id": {
      "$oid": "5aa35dbedc5673331f000001"
      },
      "name": "Harry",
      "age": 11
    }
   ```


[^_^]:
    本文使用的所有引用及链接
[operator]:manual/Manual/SQL_Grammar/operator.md#比较运算符
[operator1]:manual/Manual/SQL_Grammar/operator.md#逻辑运算符

