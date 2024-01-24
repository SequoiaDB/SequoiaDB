
聚集函数

将字段合并为一个不包含重复值的数组字段。

##语法##
***mergearrayset( \<field name\> ) as \<alias_name\>***

##参数###
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| field_name | string | 字段名  | 是 |
| alias_name | string | 别名  | 是 |
>**Note:**
>
>使用 mergearrayset 函数时，必须使用别名。

##返回值##
不包含重复值的数组。

##示例##
   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   { a: 1, b: [1, 2] }
   { a: 1, b: [2, 3] }
   ```

   * 将表中 b 字段合并为一个不包含重复值的数组字段 b 

   ```lang-javascript
   > db.exec( "select a, mergearrayset(b) as b from sample.employee group by a" )
   { "a": 1, "b": [ 1, 2, 3 ] }
   Return 1 row(s).
   ```
