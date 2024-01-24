

更新符可以实现对字段的添加、修改和删除操作，SequoiaDB 巨杉数据库支持的更新符如下：

| 更新符 | 描述 | 示例 |
|--------|------|------|
| [$inc][inc] | 增加指定字段的值 | db.sample.employee.update({$inc:{age:5,ID:1}},{age:{$gt:15}}) |
| [$set][set] | 将指定字段更新为指定的值 | db.sample.employee.update({$set:{str:"abd"}}) |
| [$unset][unset] | 删除指定的字段 | db.sample.employee.update({$unset:{name:"",age:""}}) |
| [$bit][bit] | 将指定字段的值与指定的值进行位运算 | db.sample.employee.update({$bit:{a:{xor:5}}}) |
| [$rename][rename] | 将指定字段重命名 | db.sample.employee.update({$rename:{'a':'c','b':'d'}}) |
| [$addtoset][addtoset] | 向数组中添加元素和值 | db.sample.employee.update({$addtoset:{arr:[1,3,5]}},{arr:{$exists:1}}) |
| [$pop][pop] | 删除指定数组中的最后N个元素 | db.sample.employee.update({$pop:{arr:2}}) |
| [$pull][pull]<br>[$pull_by][pull_by] | 清除指定数组中的指定值 | db.sample.employee.update({$pull:{arr:2,name:"Tom"}})<br>db.sample.employee.update({$pull_by:{arr:2,name:"Tom"}}) |
| [$pull_all][pull_all]<br>[$pull_all_by][pull_all_by] | 清除指定数组中的指定值 | db.sample.employee.update({$pull_all:{arr:[2,3],name:["Tom"]}})<br>db.sample.employee.update({$pull_all_by:{arr:[2,3],name:["Tom"]}}) |
| [$push][push] | 将给定值插入到数组中 | db.sample.employee.update({$push:{arr:1}}) |
| [$push_all][push_all] | 向指定数组中插入所有给定值 | db.sample.employee.update({$push_all:{arr:[1,2,8,9]}}) |
| [$replace][replace] | 将集合中除 _id 字段和自增字段外的文档内容全部替换 | db.sample.employee.update({$replace:{age:0,name:'default'}},{age:{$exists:0}}) |



[^_^]:
    本文使用的所有引用及链接：
[inc]:manual/Manual/Operator/Update_Operator/inc.md
[set]:manual/Manual/Operator/Update_Operator/set.md
[unset]:manual/Manual/Operator/Update_Operator/unset.md
[bit]:manual/Manual/Operator/Update_Operator/bit.md
[rename]:manual/Manual/Operator/Update_Operator/rename.md
[addtoset]:manual/Manual/Operator/Update_Operator/addtoset.md
[pop]:manual/Manual/Operator/Update_Operator/pop.md
[pull]:manual/Manual/Operator/Update_Operator/pull.md
[pull_by]:manual/Manual/Operator/Update_Operator/pull_by.md
[pull_all]:manual/Manual/Operator/Update_Operator/pull_all.md
[pull_all_by]:manual/Manual/Operator/Update_Operator/pull_all_by.md
[push]:manual/Manual/Operator/Update_Operator/push.md
[push_all]:manual/Manual/Operator/Update_Operator/push_all.md
[replace]:manual/Manual/Operator/Update_Operator/replace.md