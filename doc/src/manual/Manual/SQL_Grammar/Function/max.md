
聚集函数

用于返回记录中，指定字段名的最大值。

##语法##
***max(\<field_name\>) as \<alias_name\>***

##参数###
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| field_name | string | 字段名  | 是 |
| alias_name | string | 别名  | 是 |
>**Note:**
>
>* 使用 max 函数返回字段的最大值时，必须使用别名。

##返回值##
指定字段在记录中的最大值。

##示例##
   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   {a:0, b:2}
   {a:1, b:2}
   {a:2, b:3}
   {a:3, b:3}
   ```

   * 返回集合 sample.employee 中 a 字段的最大值 

   ```lang-javascript
   > db.exec("select max(a) as 最大值 from sample.employee")
   { "最大值": 3 }
   Return 1 row(s).
   ```