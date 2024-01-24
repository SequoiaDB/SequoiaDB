
##语法##

```lang-json
{ $addtoset: { <字段名1>: [ <值1>,<值2>,...,<值N>] ，<字段名2>: [ <值1>,<值2>,...,<值N> ], ... } }
```

##描述##

$addtoset 是向数组对象中添加元素和值，操作对象必须为数组类型的字段。$addtoset 有如下规则：

* 记录中有指定的字段名（<字段名1>,<字段名2>,...）。

 如果指定的值（[<值1>,<值2>,...,<值N>]）在记录中存在，跳过不做任何操作，只向目标数组对象中添加不存在的值。

* 记录中不存在指定的字段名。

 如果记录本身不存在指定的字段名（<字段名1>,<字段名2>,...），那么将指定的字段名和值更新到记录中。

##示例##

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2,4], age: 10, name: "Tom" }
 ```

 执行更新操作，记录中存在目标数组对象

 ```lang-javascript
 > db.sample.employee.update({ $addtoset: { arr: [1,3,5] } }, { arr: { $exists: 1 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2,4,3,5], age: 10, name: "Tom" }
 ```

 将原记录 arr 数组没有的元素 3 和 5，使用 $addtoset 之后更新到 arr 数组内。

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { name: "Mike", age: 12 }
 ```

 执行更新操作，记录中不存在目标数组对象

 ```lang-javascript
 > db.sample.employee.update({ $addtoset: { arr: [1,3,5] } }, { arr: { $exists: 0 } })
 ```

 此操作后，记录更新为：  

 ```lang-json
 { arr: [1,3,5], age: 12, name: "Mike" }
 ```

 原记录中没有数组对象 arr 字段，$addtoset 操作将 arr 字段和值更新到记录中。
