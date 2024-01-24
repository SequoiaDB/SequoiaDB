
insert into 用于向集合中插入新的数据。

##语法##
***insert into \<cs_name\>.\<cl_name\>(\<field1_name,field2_name,...\>) values(\<value1,value2,...\>)***

或者

***insert into \<cs_name\>.\<cl_name\> \<select_set\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| cs_name| string | 集合空间名  | 是 |
| cl_name| string | 集合名  | 是 |
| field1_name/field2_name | string | 字段名  | 是 |
| value1/value2|  | 字段值  | 是 |
| select_set | set | 结果集  | 是 |

##返回值##
无 

## 示例##

   * 向集合 sample.employee 中插入一条新的数据，字段名为 age 和 name，对应的值分别为(25，"Tom") 

   ```lang-javascript
   > db.execUpdate("insert into sample.employee(age,name) values(25,\"Tom\")")
   ```

   * 向集合 sample.employee2 中插入批量的数据，这些数据为集合 sample.employee 中的查询结果集 

   ```lang-javascript
   > db.execUpdate("insert into sample.employee2 select * from sample.employee")
   ```
