
offset 用于设置跳过的记录数量。

##语法##
***offset \<offset_num\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| offset_num | int32 | 跳过的记录数量  | 是 |

##返回值##
无。

## 示例##

   * 集合 sample.employee 中原始记录 

   ```lang-json
   {a:0, b:2}
   {a:1, b:2}
   {a:2, b:3}
   {a:3, b:3}
   ```

   * 跳过返回结果集的前 3 条记录 

   ```lang-javascript
   > db.exec("select * from sample.employee offset 3")
   { "_id": { "$oid": "5829aa9efa75cf4024000017" }, "a": 3, "b": 3 }
   Return 1 row(s).
   Takes 0.4503s.
   ```
