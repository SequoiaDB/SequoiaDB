
聚集函数

用于求指定字段名的平均值。

##语法##
***avg(field_name) as \<alias_name\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field_name | string | 字段名  | 是 |
| alias_name | string | 别名  | 是 |
>**Note:**
>
> * 使用 avg 函数对字段名求平均值，必须使用别名。
> * 对非数值型字段自动跳过。

##返回值##
指定字段名的平均值。

##示例##
   * 集合 sample.employee 中存在如下记录：

   ```lang-json
   { "name": "Lucy", "age": 11 }
   { "name": "Sam", "age": 8 }
   { "name": "Tom", "age": 7 }
   { "name": "James", "age": 12 }
   ```

   * 对集合 sample.employee 中 age 字段进行求平均值 

   ```lang-javascript
   > db.exec("select avg(age) as 平均年龄 from sample.employee")
   {
      "平均年龄": 9.5
   }
   Return 1 row(s).
   ```