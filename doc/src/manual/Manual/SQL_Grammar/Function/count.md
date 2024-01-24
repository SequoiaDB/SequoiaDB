聚集函数

用于计数，返回匹配指定字段名的条数。

##语法##

***count(field_name) as \<alias_name\>***

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field_name | string | 字段名 | 是 |
| alias_name | string | 别名   | 否 |

>**Note:**
>
> 使用 count 函数对字段名计数，必须使用别名

##返回值##
返回对指定字段的计数

##示例##
   * 集合 sample.employee 中记录如下：

   ```lang-json
   { "name": "tom", "age": 10 }
   { "name": "sam", "age": 11 }
   { "name": "james", "age": 13 }
   ```

   * 通过 age 字段统计集合 sample.employee 中的记录数

   ```lang-javascript
   > db.exec("select count(age) as 数量 from sample.employee")
   { "数量": 3 }
   Return 1 row(s).
   ```

   > **Note：**
   > 
   > 在 SQL 中，字段缺失则值会为 null，在统计时作为一条记录计入。

   * 对集合 sample.employee 中 age 字段进行计数

   ```lang-javascript
   > db.exec("select count(age) as 数量 from sample.employee where age isnot null")
   { "数量": 2 }
   Return 1 row(s).
   ```
