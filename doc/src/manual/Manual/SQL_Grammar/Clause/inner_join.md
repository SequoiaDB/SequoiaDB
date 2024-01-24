
inner join 用于根据两个或多个集合中的字段名之间的关系，从这些集合中查询数据。

##语法##
***\<collection1_name | (select_set1)\> as \<alias1_name\> inner join \<collection2_name | (select_set2)\> as \<alias2_name\> [on condition]***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| collection1_name/collection2_name | string | 集合名  | 是 |
| select_set1/select_set2 | set | 结果集  | 是 |
| alias1_name/alias2_name | string | 别名  | 是 |
| condition | expression | 集合之间关联条件  | 否 |

##返回值##
在集合中存在至少一个匹配时，inner join 返回匹配的记录。

##示例##

   * 集合 sample.persons 中记录如下：

   ```lang-json
   { "Id_P": 1, "LastName": "Adams", "FirstName": "John", "Address": "Oxford Street", "City": "London" }
   { "Id_P": 2, "LastName": "Bush", "FirstName": "George", "Address": "Fifth Avenue", "City": "New York" }
   { "Id_P": 3, "LastName": "Carter", "FirstName": "Thomas", "Address": "Changan Street", "City": "Beijing" }
   ```

   * 集合 sample.orders 中记录如下：

   ```lang-json
   { "Id_O": 1, "OrderNo": 77895, "Id_P": 3 }
   { "Id_O": 2, "OrderNo": 44678, "Id_P": 3 }
   { "Id_O": 3, "OrderNo": 22456, "Id_P": 1 }
   { "Id_O": 4, "OrderNo": 24562, "Id_P": 1 }
   { "Id_O": 5, "OrderNo": 34764, "Id_P": 65 }
   ```


   *  列出所有人的订购 

   ```lang-javascript
   > db.exec("select t1.LastName, t1.FirstName, t2.OrderNo from sample.persons as t1 inner join sample.orders as t2 on t1.Id_P=t2.Id_P")
   { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 }
   { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 }
   { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 }
   { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 }
   Return 4 row(s).
   ```

   >**Note:**
   >
   > * 不能包含非联合条件，如下写法是错误的：
   >
   >   ```lang-javascript
   >   > db.exec( "select T1.a, T2.b from sample.employee1 as T1 inner join sample.employee2 as T2 on T1.a < 10" ) 
   >   ```
   > * 不能在 inner join 本层使用 select * 语句。
