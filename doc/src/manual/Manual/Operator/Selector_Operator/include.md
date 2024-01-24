
##语法##

```lang-json
{ <字段名1>: { $include: <0|非0> }, <字段名2>: { $include: <0|非0>, ... } }
```

##说明##

$include 可以指定选择或移除某个字段。

| 值  | 描述 |
| --- | ---- |
| 非 0 | 选择字段 |
| 0   | 移除字段 |

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert( { "_id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 }, { "name": "WangErmazi", "age": 18 } ] } )
```

* 查询集合 sample.employee，指定选择 students 字段

  ```lang-javascript
  > db.sample.employee.find( {}, { "students": { "$include": 1 } } )
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
      ]
  }
  Return 1 row(s).
  ```

* 查询集合 sample.employee，指定移除 students 字段

  ```lang-javascript
  > db.sample.employee.find( {}, { "students": { "$include": 0 } } )
  {
      "_id": 1,
      "class": 1
  }
  ```