
as 用于为集合名、字段名或结果集指定别名（alias）。

##语法##
***\<cs_name.cl_name | (select_set) | field_name\> as \<alias_name\>***

##参数##
| 参数名          | 参数类型 | 描述       | 是否必填 |
|-----------------|----------|------------|----------|
| cs_name | string | 集合空间名  | 否 |
| cl_name | string | 集合名  | 否 |
| select_set | set | 查询结果集  | 否 |
| field_name | string | 字段名  | 否 |
| alias_name | string | 别名  | 是 |

##示例##
   * 集合 sample.employee 中存在如下记录 

   ```lang-json
   { "name": "Lucy", "age": 11 }
   { "name": "Sam", "age": 8 }
   { "name": "Tom", "age": 7 }
   { "name": "James", "age": 12 }
   ```

   * 集合别名  

   ```lang-javascript
   > db.exec("select T1.age, T1.name from sample.employee as T1 where T1.age>10")
   { "name": "Lucy", "age": 11 }
   { "name": "James", "age": 12 }
   Return 2 row(s).
   ```

   * 字段别名 

   ```lang-javascript
   > db.exec("select age as 年龄 from sample.employee where age>10")
   { "年龄": 11 }
   { "年龄": 12 }
   Return 2 row(s).
   ```

   * 结果集别名 

   ```lang-javascript
   > db.exec("select T.age, T.name from (select age, name from sample.employee) as T where T.age>10")
   { "name": "Lucy", "age": 11 }
   { "name": "James", "age": 12 }
   Return 2 row(s).
   ```
