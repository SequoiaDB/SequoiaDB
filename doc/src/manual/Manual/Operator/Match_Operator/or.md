
##语法##

```lang-json
{ $or: [ { <表达式1> }, { <表达式2> }, ... ] }
```

##描述##

$or 是一个逻辑“或”操作，用于选择满足表达式（ [ { <表达式1> }, { <表达式2> }, ... ] ）其中一个表达式的记录。只要有一个表达式的计算结果为 true，记录就会返回。

##示例##

* 查询集合 sample.employee 下 name 字段值为“Tom”，且 age 字段值为 20 或 price 字段值小于 10 的记录

  ```lang-javascript
  > db.sample.employee.find( { name: "Tom", $or: [ { age: 20 }, { price: { $lt: 10 } } ] } )
  ```

* $or 匹配嵌套对象中的字段名，选择 age 字段值小于20或者嵌套对象 snapshot 中的 type 字段值为“system”的记录，并使用 $inc 更新这些记录的 salary 字段值

  ```lang-javascript
  > db.sample.employee.update( { $inc: { salary: 200 } }, { $or: [ { age: { $lt: 20 } }, { "snapshot.type": "system" } ] } )
  ```
