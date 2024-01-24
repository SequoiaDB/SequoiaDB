
##语法##

```lang-json
{ $not: [ { <表达式1> }, { <表达式2> }, ... } ] }
```

##描述##

$not 是一个逻辑“非”操作，用于选择不匹配表达式（ [ <表达式1>, <表达式2>, ... ] ）的记录。只要不满足其中的任意一个表达式，记录就会返回。

## 示例##

* 查询集合 sample.employee 下 age 字段值不等于 20 或 price 字段值不小于 10 的记录

  ```lang-javascript
  > db.sample.employee.find( { $not: [ { age: 20 }, { price: { $lt: 10 } } ] } )
  ```
