
聚集函数

将集合中多条记录中的相同字段的值合并到一个没有重复值的数组中。

##语法##
***addtoset(\<field_name\>)***

##参数##
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| field_name | string | 其值将被合并的字段名  | 是 |

##返回值##
包含 *field_name* 字段所有不同取值的数组。

##示例##

   * 集合 sample.employee 中原始记录如下所示：

   ```lang-json
   {a:1, b:1}
   {a:2, b:2}
   {a:2, b:3}
   {a:2, b:3}
   ```

   * 本例将以 a 字段分组，得到集合 sample.employee 中记录的 a 字段相同时，所有 b 字段的取值 

   ```lang-javascript
   > db.exec("select a, addtoset(b) as b from sample.employee group by a")
   {
     "a": 1,
     "b": [
       1
     ]
   }
   {
     "a": 2,
     "b": [
       2,
       3
     ]
   }
   Return 2 row(s).
   ```