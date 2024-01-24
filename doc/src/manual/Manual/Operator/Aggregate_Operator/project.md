
##语法##

```lang-json
{ $project: { <字段名1：0 | 1 | "$新字段名1">, [字段名2: 0 | 1 | "$新字段名2", ... ] } }
```

##描述##

$project 类似 SQL 中的 select 语句，通过 $project 操作可以从记录中筛选出所需字段。$project 规则如下：

- 字段值为 1，表示选出；字段值为 0，表示不选
- 当 $project 指定多个字段，且这些字段值同时为 0 时表示不进行筛选，输出所有字段
- 对嵌套对象使用点操作符（.）引用字段名
- 如果记录不存在所选字段，则该字段值输出为 null
- 通过“$新字段名”可以实现字段重命名

##示例##

- 使用 $project 快速地从结果集中选取所需字段

 ```lang-javascript
 > db.sample.employee.aggregate({ $project: { title: 0, author: 1 } })
 ```

 此操作是选出 author 字段，而 title 字段在结果集中不输出。

- 使用 $project 重命名字段名

 ```lang-javascript
 > db.sample.employee.aggregate({ $project : { author: 1, name: "$studentName", dep: "$info.department" } })
 ```

 此操作将字段名 studentName 重命名为 name 输出，将 info 对象中的子对象 department 字段重命名为 dep。对嵌套对象，字段引用使用点操作符（.）指向。

- 使用 $project 选择输出字段，然后使用 $match 按条件匹配记录

 ```lang-javascript
 > db.sample.employee.aggregate({ $project: { score: 1, author: 1 } }, { $match: { score: { $gt: 80 } } })
 ```

 此操作使用 $project 输出所有记录的 score 和 author 字段值，然后按 $match 输出匹配条件的记录。

 > **Note:**
 >
 > 由于 $project 选取了输出字段名，所以 $match 中字段名必须是 $project 中选出的字段名。
