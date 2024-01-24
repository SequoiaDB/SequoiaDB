
##语法##

$match 与 [db.collectionspace.collection.find()][find] 方法中的 cond 参数完全相同。

##描述##

通过 $match 可以从集合中选择匹配条件的记录。

##示例##

- 使用 $match 执行简单的匹配

 ```lang-javascript
 > db.sample.employee.aggregate({ $match: { $and: [ { score: 80 }, { "info.name": { $exists: 1 } } ] } })
 ```

 该操作表示从集合 sample.employee 中返回符合条件 score 等于 80 且 info 对象中的子对象 name 字段存在的记录。

- 使用 $match 匹配符合条件的记录，然后使用 $group 对结果集分组，最后使用 $project 输出结果集中指定的字段名

 ```lang-javascript
 > db.sample.employee.aggregate({ $match: { $and: [ { score: 80 }, { "info.name": { $exists: 1 } } ] } }, { $group: { _id: "$major" } }, { $project: { major: 1, dep: 1 } })
 ```

 该操作首先集合 sample.employee 中返回符合条件 score 等于 80 且 info 对象中的子对象 name 字段存在的记录，然后按  major 字段进行分组，最后选择输出结果集中的 major 和 dep 字段。


[^_^]:
   本文使用的所有引用及链接

[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
