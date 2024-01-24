
##语法##

```lang-json
{ $set: { <字段名1>: <值1>, <字段名2>: <值2>, ... } }
```

##描述##

$set 操作是将指定的“<字段名>”更新为指定的“<值>”。如果原记录中没有指定的字段名，那将字段名和值填充到记录中；如果原记录中存在指定的字段名，那么将字段名的值更新为指定的值。

其中“<值>”支持以下几种格式：

* 任意类型的值，如： 

  ```lang-json
  { $set: { field: 2 } }
  ```
  
* 通过[字段操作符][field]指定的原始记录中的某字段，如：

  ```lang-json
  { $set: { field: { $field: "fieldName" } } }
  ```
  
  如果 fieldName 字段在原始记录中不存在，则删除目标字段 field。

##示例##

* 选择集合 sample.employee 下不存在 age 字段的记录，使用 $set 更新这些记录

 ```lang-javascript
 > db.sample.employee.update({ $set: { age: 5, ID: 10 } }, { age: { $exists: 0 } })
 ```

* 更新集合 sample.employee 下的所有记录，使所有记录的字段 str 值更新为“abc”

 ```lang-javascript
 > db.sample.employee.update({ $set: { str: "abc" } })
 ```

* 使用 $set 更新嵌套数组对象里面的元素，字段名 arr 在集合 sample.employee 中是一个嵌套数组对象，例如有两条记录`{arr:[1,2,3],name:"Tom"}`,`{name:"Mike",age:20}`第二条记录没有 arr 字段名

 ```lang-javascript
 > db.sample.employee.update({ $set: { "arr.1": 4 } }, { name: { $exists: 1 } })
 ```

 此操作是选择含有 name 字段的所有记录，然后使用 $set 更新这些记录的数组对象 arr；如果原记录中没有数组对象 arr，使用 $set 会将 arr 字段以嵌套对象的方式插入到记录中

 上述两条记录更新之后为：

 ```lang-json
 { arr: [1,4,3], name: "Tom" }, { arr: { "1": 4 }, name: "Mike", age: 20 }
 ```

* 对于以上两条记录，在 $set 中使用一个字段更新另一个字段

 ```lang-javascript
 > db.sample.employee.update({ $set: { weight: { $field: "age" }}}, {name:"Mike"})
 > db.sample.employee.update({ $set: { name: { $field: "notexist" }}})
 ```

 第一条语句中使用的字段 age 在第二条记录中是存在的，因此可以完成更新。第二条语句中使用的字段 notexist 在原始记录中是不存在的，因此将目标字段 name 删除

 上述两条记录更新之后为：

 ```lang-json
 { "arr": [ 1, 4, 3 ] }, { "age": 20, "arr": { "1": 4 }, "weight": 20 }
 ```


[^_^]:
     本文使用的引用及链接
[field]:manual/Manual/Operator/Field_Operator/field.md

