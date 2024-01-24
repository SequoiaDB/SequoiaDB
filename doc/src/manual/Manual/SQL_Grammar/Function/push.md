
聚集函数

将多个值合并为一个数组。

##语法##
***push( \<field_name\> ) as \<alias_name\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field_name | string | 字段名  | 是 |
| alias | string | 别名  | 是 |
>**Note:**
>
>使用 push函数时，必须使用别名。

##返回值##
包含记录中相同字段的值的数组。

##示例##

   * 集合 sample.employee 中原始记录如下：

   ```
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2, "c": 2 }
   { "a": 2, "b": 3 }
   ```

   * 将表中 b 字段的值合并为一个数组

   ```lang-javascript
   > db.exec( "select a, push(b) as b from sample.employee group by a" )
   { "a": 1, "b": [ 1 ] }
   { "a": 2, "b": [ 2, 2, 3 ] }
   Return 2 row(s).
   ```
