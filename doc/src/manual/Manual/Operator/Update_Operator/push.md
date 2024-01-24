
##语法##

```lang-json
{ $push: {<字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$push 将给定数值<值1>插入到目标数组<字段名1>中，操作对象必须为数组类型的字段。如果记录中不存在指定的字段名，将指定的字段名以数组对象的形式推入到记录中并填充其指定的数值；如果记录中存在指定的字段名，且字段名存在指定的数值，指定的数值也会被推入到记录中。

其中<值>支持以下几种格式：

* 任意类型的常量值

  ```lang-json
  { $push: { arr: 1 } }
  ```
  
* 通过[字段操作符][field]指定的原始记录中的某字段

  ```lang-json
  { $push: { arr: { $field: "fieldName" } } }
  ```
  
  >**Note:**
  >
  > 如果 fieldName 字段在原始记录中不存在，则不做任何操作。

##示例##


* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2,4], age: 10, name: ["Tom","Mike"] }
 ```

 向集合中的 arr 数组对象推入数值 1

 ```lang-javascript
 > db.sample.employee.update({ $push: { arr: 1 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2,4,1], age: 10, name: ["Tom","Mike"] }
 ```

  >**Note:**
  >
  > 虽然原来 arr 中有元素 1，使用 $push 操作符，还是会将元素 1 推入到 arr 数组对象中。

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { arr: [1,2], age: 20 }
 ```

 向集合中推入不存在的数组对象和值

 ```lang-javascript
 > db.sample.employee.update({ $push: { name: "Tom" } }, { name: { $exists: 0 } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [1,2], age: 20, name: ["Tom"] }
 ```

  >**Note:**
  >
  > 原记录中不存在数组对象 name，使用 $push 操作符，会将 name 以数组对象的形式推入到记录中。

* 将上述集合中 age 字段的值推入到 arr 字段中

 ```lang-javascript
 > db.sample.employee.update({ $push: { arr: { $field: "age" } } })
 ```

 此操作后，记录更新为：

 ```lang-json
 { arr: [ 1, 2, 20 ], age: 20, name: [ "Tom" ] }
 ```


[^_^]:
    本文使用的所有引用及链接
[field]:manual/Manual/Operator/Field_Operator/field.md
