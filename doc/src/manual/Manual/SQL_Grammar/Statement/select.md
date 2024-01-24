
select 用于从集合中选取数据，结果被存储在一个结果集中。

##语法##

***select \< * | field1_name,field2_name,... | [mathematical_expression][expression] \> from \<cs_name.cl_name | statement\> [clause]***

如：

***select * from \<cs_name\>.\<cl_name\>***

***select \<field1_name,field2_name\> from \<cs_name\>.\<cl_name\>***

***select * from \<cs_name\>.\<cl_name\> [where][where1] \<comparison_expression\>***

***select * from \<cs_name\>.\<cl_name\> [where][where1]  \<comparison_expression\> [order by][order_by] \<field1_name\>*** 

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| field_name/field2_name  | string     | 字段名         | 否 |
| mathematical_expression | expression | [算术表达式][expression] | 否 |
| cs_name                 | string     | 集合空间名     | 是 |
| cl_name                 | string     | 集合名         | 是 |
| statement               | statement  | 语句           | 否 |
| clause                  | expression | 子句             | 否 |

>**Note:**
>
> * 可以选择 where，group by，order by，limit，offset 等子句，对要选择的记录做控制。
>
> * 如果查询源不为集合，则本层查询中所有字段均需要引用别名（\* 除外），例如：select T.a , T.b from (select \* from sample.employee) as T where T.a < 10 。
>
> * 子查询必须包含别名。子查询中出现的别名只作用于上一层。

##返回值##

记录集。

##示例##

   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   { age: 10 }
   { age: 10, name: "Tom" }
   ```

   * 选择指定的字段名返回，如果某条符合条件的记录没有指定的字段名，那么返回记录中它的值为 null 

   ```lang-javascript
   > db.exec( "select age,name from sample.employee" )
   { "age": 10, "name": "Tom" }
   { "age": 10, "name": null }
   Return 2 row(s).
   ```

   * 返回集合中的所有记录的所有字段名

   ```lang-javascript
   > db.exec( "select * from sample.employee" )
   { _id: { $oid: "5811ad16751e72e564000016" },  age: 10,  name: "Tom" }
   { _id: { $oid: "5811ad1a751e72e564000017" },  age: 10 }
   Return 2 row(s).
   ```


[^_^]:
    本文使用的所有引用及链接
[expression]:manual/Manual/SQL_Grammar/expression.md#算术表达式
[where1]:manual/Manual/SQL_Grammar/Clause/where.md
[order_by]:manual/Manual/SQL_Grammar/Clause/order_by.md

