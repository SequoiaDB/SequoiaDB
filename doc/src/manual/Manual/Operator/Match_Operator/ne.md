
##语法##

```lang-json
{ <字段名>: { $ne: <值> } }
```

##描述##

$ne 选择满足“<字段名>”的值不等于（!=）指定“<值>”的记录。

> **Note:**
>
> $ne 不能匹配给定字段名不存在的记录，如果用户需要匹配字段名不存在的记录，需使用 [$exists](manual/Manual/Operator/Match_Operator/exists.md)。

##示例##

* 查询集合 sample.employee 中 age 字段值不等于 20 的记录

  ```lang-javascript
  > db.sample.employee.find( { "age": { "$ne": 20 } } )
  ```

* $ne 匹配嵌套对象中的字段名，使用 update() 方法更新嵌套对象 service 中的 type 字段值不等于 15 的记录，将这些记录的 age 字段值设定为 25

  ```lang-javascript
  > db.sample.employee.update( { "$set": { "age": 25 } }, { "service.type": { "$ne": 15 } } )
  ```
