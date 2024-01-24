
##语法##

```lang-json
{ <字段名>: { $lte: <值> } }
```

##描述##

$lte 选择满足“<字段名>”的值小于等于（\<=）指定“<值>”的记录。

##示例##

* 查询集合 sample.employee 中字段名为 age 的值小于等于 20 的记录

  ```lang-javascript
  > db.sample.employee.find( { age: { $lte: 20 } } )
  ```

* $lte 匹配一个嵌套对象中的字段名，使用 update() 方法更新嵌套对象 service 中的 ID 字段值小于等于 2 的记录，将这些记录的 age 字段值设定为 25

  ```lang-javascript
  > db.sample.employee.update( { $set: { age: 25 } }, { "service.ID": { $lte: 2 } } )
  ```
