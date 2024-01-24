
##语法##

```lang-json
{ $replace: {<字段名1>: <值1>, <字段名2>: <值2>, ...}}
```

##描述##

$replace 用于将集合记录替换为指定的 {<字段名1>: <值1>, <字段名2>: <值2>, ...}。当指定的内容不包含字段 _id 或自增字段时，该类字段将被保留。但在自增字段处于嵌套结构的情况下，该自增字段将无法保留。

##示例##

集合 sample.employee 存在如下记录，其中字段 ID 为自增字段：

```lang-text 
{"_id": {"$oid": "628f2bfa8b3bea6048effcb6"}, "ID": 1, "name": "Any", "age": 18}
{"_id": {"$oid": "628f2c048b3bea6048effcb7"}, "ID": 2, "name": "Joy", "age": 20}
```

将字段 age 为 20 的记录替换为 {"name": "default", "age": 0}

```lang-javascript
> db.sample.employee.update({$replace: {name: "default", age: 0}}, {age: {$et: 20}})
```

查看替换后的记录

```lang-javascript
> db.sample.employee.find()
{
  "_id": {
    "$oid": "628f2bfa8b3bea6048effcb6"
  },
  "ID": 1,
  "name": "Any",
  "age": 18
}
{
  "_id": {
    "$oid": "628f2c048b3bea6048effcb7"
  },
  "ID": 2,
  "age": 0,
  "name": "default"
}
```
