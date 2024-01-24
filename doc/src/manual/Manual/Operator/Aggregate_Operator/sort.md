
##语法##

```lang-json
{ $sort: { 字段1: -1 | 1 [, 字段2: -1 | 1, ... ] } }
```

##描述##

$sort 用于指定结果集的排序规则。对嵌套对象使用点操作符（.）引用字段名。

- 字段取值为 -1 表示降序排序
- 字段取值为 1 表示升序排序

##示例##

从集合 sample.employee 中读取记录，并以 score 的字段值进行降序排序；当记录间 score 字段值相同时，则以 name 字段值进行升序排序

```lang-javascript
> db.sample.employee.aggregate({ $sort: { score: -1, name: 1 } })
```


