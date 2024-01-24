
##语法##

```lang-json
{ $pull_all_by: { <字段名1>: [ <值1>,<值2>,...,<值N> ], <字段名2>: [ <值1>,<值2>,...,<值N> ], ... } }
```

##描述##

$pull_all_by 用于删除数组中指定的元素，与 [$pull_all][pull_all] 功能类似，但两者存在以下区别：

- $pull_all_by：当<值>为对象时，只要<值>中的字段存在于数组记录中，则匹配成功并删除对应的数组元素；

- $pull_all：当<值>为对象时，需要<值>中每个字段与数组元素中的每个字段相同，才能匹配成功并删除对应的数组元素。

##示例##

* 集合 sample.employee 存在如下记录：

   ```lang-json
   {a:[{id:1,num:1}, {id:2,num:2}, {id:3,num:3}, {id:4,num:4}, {id:2}]}
   ```

   * 使用 $pull_all 对 a 字段进行如下操作：


     ```lang-json
     > db.sample.employee.update({$pull_all:{a:[{id:2}]}})
     ```

     此操作后，记录更新结果如下，只删除了字段与<值>完全相同的元素{id:2}：

     ```lang-json
     {a:[{id:1,num:1}, {id:2,num:2}, {id:3,num:3}, {id:4,num:4}]}
     ```

   * 使用 $pull_all_by 对 a 字段进行如下操作：

     ```lang-json
     > db.sample.employee.update({$pull_all_by:{a:[{id:2}]}})
     ```

     此操作后，记录更新结果如下，与<值>存在相同字段的元素 { id: 2, num: 2 } 和 { id: 2 } 被删除：

     ```lang-json
     {a:[{id:1,num:1}, {id:3,num:3}, {id:4,num:4}]}
     ```

* 集合 sample.employee 存在如下记录：

   ```lang-json
   {a:[1,2,3,2], b:[4,5,6]}
   ```

   操作 a 字段，删除字段值中为 1 或 2 的元素；操作 b 字段，删除字段值中为 5 的元素

   ```lang-javascript
   > db.sample.employee.update({$pull_all_by:{a:[1,2],b:[5]}})
   ```

   此操作后，记录更新如下：

   ```lang-json
   {a:[3], b:[4,6]}
   ```



[^_^]:
    本文使用的所有引用及链接
[pull_all]:manual/Manual/Operator/Update_Operator/pull_all.md