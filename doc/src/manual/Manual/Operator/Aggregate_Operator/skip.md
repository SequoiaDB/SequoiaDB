
##语法##

```lang-json
{ $skip: <记录数> }
```

##描述##

$skip 参数控制结果集的开始点，即跳过结果集中指定条数的记录。如果跳过的记录数大于总记录数，返回 0 条记录。

##示例##

- 返回结果时跳过前 10 条记录

 ```lang-javascript
 > db.sample.employee.aggregate( { $skip: 10 } )
 ```

 该操作表示从集合 sample.employee 中读取记录，并跳过前面 10 条，从第 11 条记录开始返回。
