
##语法##

```lang-json
{ <字段名>: { $nin: [ <值1>, <值2>, ... ] } }
```

##描述##

选择集合中“<字段名>”值不等于给定数组（ [ <值1>, <值2>, ... ] ）中任意一个值的记录；如果“<字段名>”本身是数组类型，那么选择“<字段名>”中任意一个值都不等于给定数组（ [ <值1>, <值2>, ... ] ）中值的记录。

> **Note:**
>
> $nin 不能匹配给定字段名不存在的记录，如果用户需要匹配字段名不存在的记录，需使用 [$exists][exists]。

##示例##

* 查询集合 sample.employee 下 age 字段的值不等于 20 和 25 的记录

  ```lang-javascript
  > db.sample.employee.find( { age: { $nin: [ 20, 25 ] } } )
  ```

* $nin 匹配数组对象中的元素，查询集合 sample.employee 中存在字段 name 且其元素不包含“Tom”和“Mike”的记录，并将这些记录的 age 字段删除

  ```lang-javascript
  > db.sample.employee.update( { $unset: { age: "" } }, { name: { $nin: [ "Tom", "Mike" ] } } )
  ```


- 当给定数组只有一个值时，即{ <字段名>: { $nin: [ <值> ]  }，等价于 { <字段名>: { $ne: <值> } }

   ```lang-javascript
   > db.sample.employee.find( { age: { $nin: [ 20 ] } } )
   ```

   等价于

   ```lang-javascript
   > db.sample.employee.find( { age: { $ne: 20 } } )
   ```


[^_^]:
    本文使用的所有引用及链接
[exists]:manual/Manual/Operator/Match_Operator/exists.md