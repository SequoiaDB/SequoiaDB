
##语法##
```lang-json
{ $pop: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$pop 操作是删除指定数组对象（<字段名1>,<字段名2>,...）最后 N 个元素，N 的大小由“<值>"决定。如果记录中不存在指定的数组对象，则不做任何操作；如果 N 大于数组对象的长度，数组对象的长度更新为 0，即所有元素被删除；如果 N<0，则从数组起始删除 -N 个元素。

其中“<值>”支持以下两种格式：

* 数值常量

  ```lang-json
  { $pop: { arr: 2 } }
  ```
  
* 通过[字段操作符][field]指定的原始记录中某数值类型的字段

  ```lang-json
  { $pop: { arr: { $field: "fieldName" } } }
  ```
  
  > **Note:**
  >
  > 如果 fieldName 字段在原始记录中不存在，则不做任何操作；如果存在但不是数值类型，则报错。

##示例##

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2,3,4], age: 20, name: "Tom" }
 ```

 删除集合中数组对象 arr 的最后两个元素

 ```lang-javascript
 > db.sample.employee.update({ $pop: { arr: 2 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2], age:20, name: "Tom" }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2,3,4], age: 20, name: "Tom" }
 ```

 删除集合中数组对象 arr 的最后十个元素

 ```lang-javascript
 > db.sample.employee.update({ $pop: { arr: 10 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [], age: 20, name: "Tom" }
 ```

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2,3,4], age: 20, name: "Tom" }
 ```

 删除集合中数组对象arr的前两个元素，即设置 N 的值为 -2

 ```lang-javascript
 > db.sample.employee.update({ $pop: { arr: -2 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [3,4], age: 20, name: "Tom" }
 ```

* 在以上记录中新增一个字段 m，其值为 2，然后使用该字段的值对 arr 字段执行 $pop 操作

 ```lang-javascript
 > db.sample.employee.update({ $set: { m: 2 } })
 > db.sample.employee.update({ $pop: { arr: { $field: "m" } } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [], age: 20, m: 2, name: "Tom" }
 ```


[^_^]:
    本文使用的所有引用及链接
[field]:manual/Manual/Operator/Field_Operator/field.md