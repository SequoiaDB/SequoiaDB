
聚集函数

用于求和。

##语法##
***sum( \<field_name\> ) as \<alias_name\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field_name | string | 字段名  | 是 |
| alias_name | string | 别名  | 是 |
>**Note:**
>
> * 使用 sum 函数对字段求和，必须使用别名。
> * 对非数值型字段自动跳过。

##返回值##
对指定字段所有值的求和。

##示例##

   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   { age: 1 }
   { age: 2 }
   { age: 3 }
   ```

   * 对集合 employee 中 age 字段求和 

   ```lang-javascript
   > db.exec( "select sum(age) as 年龄总和 from sample.employee" )
   { "年龄总和": 6.0 }
   Return 1 row(s).
   ```