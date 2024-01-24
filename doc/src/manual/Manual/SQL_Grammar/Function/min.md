
聚集函数

用于返回最小值。

##语法##
***min( \<field_name\> ) as \<alias_name\>***

##参数###
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| field_name | string | 字段名  | 是 |
| alias_name | string | 别名  | 是 |
>**Note:**
>
>使用 min 函数时，必须使用别名。

##返回值##
指定字段在记录中的最小值。

## 示例##
   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   {a:0, b:2}
   {a:1, b:2}
   {a:2, b:3}
   {a:3, b:3}
   ```

   * 返回集合 sample.employee 中 a 字段的最小值 

   ```lang-javascript
   > db.exec("select min(a) as 最小值 from sample.employee")
   { "最小值": 0 }
   Return 1 row(s).
   ```