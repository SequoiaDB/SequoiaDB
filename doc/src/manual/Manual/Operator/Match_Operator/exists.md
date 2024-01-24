
##语法##

```lang-json
{ <字段名>: { $exists: <0|1> } }
```

##描述##

匹配集合中是否存在指定“<字段名>”的记录。

| 值  | 作用 |
| --- | ---- |
| 0   | 匹配“<字段名>”不存在的记录  |
| 1   | 匹配“<字段名>”存在的记录 |

## 示例##

* 选择集合 sample.employee 中存在字段 age 的记录

  ```lang-javascript
  > db.sample.employee.find( { "age": { "$exists": 1 } } )
  ```

* 选择集合 sample.employee 中嵌套对象 content 不存在 name 字段的记录

  ```lang-javascript
  > db.sample.employee.find( { "content.name": { "$exists": 0 } } )
  ```
