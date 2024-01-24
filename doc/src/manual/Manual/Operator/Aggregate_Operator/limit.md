##语法##

```lang-json
{ $limit: <返回记录数> }
```

##描述##

$limit 实现在结果集中限制返回的记录条数。如果指定的记录条数大于实际的记录总数，那么返回实际的记录总数。

##示例##

* 限制返回结果集中的前10条记录

 ```lang-javascript
 > db.sample.employee.aggregate( { $limit : 10 } )
 ```

 该操作表示集合 sample.employee 中读取前 10 条记录。

