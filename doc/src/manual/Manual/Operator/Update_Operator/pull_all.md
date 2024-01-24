
##语法##

```lang-json
{ $pull_all: { <字段名1>: [ <值1>, <值2>, ..., <值N> ], <字段名2>: [ <值1>, <值2>, ..., <值N> ], ... } }
```

##描述##

$pull_all 用于删除数组中指定的元素，与 [$pull][pull] 功能类似，但 $pull 只能匹配字段中的一个值，$pull_all 能匹配字段中的多个值。

执行一次 $pull_all，如

```lang-json
{ $pull_all: { <字段名1>: [ <值1>, <值2>, ..., <值N> ] } }
```

相当于执行多次 $pull

```lang-json
{ $pull: { <字段名1>: <值1> } }
{ $pull: { <字段名1>: <值2> } }
...
{ $pull: { <字段名1>: <值N> } }
```

##示例##

* 集合 sample.employee 存在如下记录：

   ```lang-json
   {arr:[1,2,4,5 ], age:10, name:["Tom","Mike"]}
   ```

   操作 arr 字段，删除字段值中为 2 或 3 的元素；操作 name 字段，删除字段值中为"Tom"的元素

   ```lang-javascript
   > db.sample.employee.update({$pull_all:{arr:[2,3], name:["Tom"]}})
   ```

   此操作后，记录更新如下：

   ```lang-json
   {arr:[ 1, 4, 5 ], age:10, name:["Mike"]}
   ```

* 集合 sample.employee 存在如下记录：

   ```lang-json
   {arr:[1,3,4,5], age:10, name:["Tom","Mike"]}
   ```

   操作 arr 字段，删除字段值中为 4 或 5 的元素
 
   ```lang-javascript
   > db.sample.employee.update({$pull_all:{arr:[4,5]}})
   ```

   此操作后，记录更新如下：

   ```lang-json
   {arr:[1,3], age:10, name:["Tom","Mike"]}
   ```


[^_^]:
    本文使用的所有引用及链接
[pull]:manual/Manual/Operator/Update_Operator/pull.md