
##语法##

```lang-json
{ <字段名1>: { $default: <默认值1> }, <字段名2>: { $default: <默认值2>, ... } }
```

##描述##

选择某个字段，当字段不存在时返回默认值，可简写为：

```lang-json
{ <字段名1>: <默认值1>, <字段名2>: <默认值2>, ... }
```

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert( { "_id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 },{ "name": "WangErmazi", "age": 18 } ] } )
```

查询集合 sample.employee 的记录，指定返回字段 students 和 teacher，并且设置默认值

```lang-javascript
> db.sample.employee.find( {}, { "students": [], "teacher": { "$default": "Mr Liu" } } )
{
    "students": [
      {
        "name": "ZhangSan",
        "age": 18
      },
      {
        "name": "LiSi",
        "age": 19
      },
      {
        "name": "WangErmazi",
        "age": 18
      }
    ],
    "teacher": "Mr Liu"
}
Return 1 row(s).
```
