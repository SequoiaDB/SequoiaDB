
##语法##

```lang-json
{ $push_all: {<字段名1>:[<值1>,<值2>,...,<值N>], <字段名2>: [<值1>,<值2>,...,<值N>], ... } }
```

##描述##

$push_all 表示向指定字段（<字段名1>）推入每一个指定值（[<值1>,<值2>,...,<值N>]）。指定字段的值必须为数组类型，无论记录中是否存在该字段，指定值都将全部推入该字段中。

> **Note:**
>
> 如果指定字段的值不为数组类型，则不会更新指定字段。 

##示例##

* 集合 sample.employee 存在如下记录：

   ```lang-json
   {arr:[1,2,4,5], age:10, name:["Tom","Mike"]}
   ```

   向集合中的字段 arr 推入数组[1,2,8,9]

   ```lang-javascript
   > db.sample.employee.update({$push_all:{arr:[1,2,8,9]}})
   ```

   此操作后，即使字段 arr 存在值 1 和 2，也将数组[1,2,8,9]全部推入 arr 中

   ```lang-json
   {arr:[1,2,4,5,1,2,8,9], age:10, name:["Tom","Mike"]}
   ```

* 集合 sample.employee 存在如下记录：

   ```lang-json
   {arr:[1,3,4,5], age:10}
   ```

   向集合中推入不存在的字段 name

   ```lang-javascript
   > db.sample.employee.update({$push_all:{name:["Tom","John"]}}, {name:{$exists: 0}})
   ```

   此操作后，记录更新如下：

   ```lang-json
   {arr:[1,3,4,5], age:10, name:["Tom","John"]}
   ```
