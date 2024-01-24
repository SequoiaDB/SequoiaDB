

匹配符可以指定匹配条件，使查询仅返回符合条件的记录；还可以与[函数操作][overview]配合使用，以实现更复杂的匹配操作。

匹配符列表如下：

| 匹配符 | 描述 | 示例 |
| ------ | ---- | ---- |
| [$gt][gt]               | 大于           | db.sample.employee.find( { age: { $gt: 20 } } )                       |
| [$gte][gte]             | 大于等于       | db.sample.employee.find( { age: { $gte: 20 } } )                      |
| [$lt][lt]               | 小于           | db.sample.employee.find( { age: { $lt: 20 } } )                       |
| [$lte][lte]             | 小于等于       | db.sample.employee.find( { age: { $lte: 20 } } )                      |
| [$ne][ne]               | 不等于         | db.sample.employee.find( { age: { $ne: 20 } } )                       |
| [$in][in]               | 集合内存在     | db.sample.employee.find( { age: { $in: [ 20, 21 ] } } )                  |
| [$nin][nin]             | 集合内不存在   | db.sample.employee.find( { age: { $nin: [ 20, 21 ] } } )                 |
| [$all][all]             | 全部           | db.sample.employee.find( { age: { $all: [ 20, 21 ] } } )                 |
| [$and][and]             | 与             | db.sample.employee.find( { $and: [ { age: 20 }, { name: "Tom" } ] } )       |
| [$not][not]             | 非             | db.sample.employee.find( { $not: [ { age: 20 }, { name: "Tom" } ] } )         |
| [$or][or]               | 或             | db.sample.employee.find( { $or: [ { age: 20 }, { name: "Tom" } ] } )          |
| [$exists][exists]       | 存在           | db.sample.employee.find( { age: { $exists: 1 } } )                    |
| [$elemMatch][elemMatch] | 元素匹配       | db.sample.employee.find( { content: { $elemMatch: { age: 20 } } } )                |
| [$+标识符][identifier]  | 数组元素匹配   | db.sample.employee.find( { "array.$2": 10 } )                      |
| [$regex][regex]         | 正则表达式     | db.sample.employee.find( { str: { $regex:  'dh, * fj', $options:'i' } } ) |
| [$mod][mod]             | 取模匹配       | db.sample.employee.find( { "age": { "$mod": [ 5, 3 ] } } ) |
| [$et][et]               | 相等匹配       | db.sample.employee.find( { "id": { "$et": 1 } } )       |
| [$isnull][isnull]       | 选择集合中指定字段是否为空或不存在 |  db.sample.employee.find( { age: { $isnull: 0 } } )  |


数组属性操作

|数组属性操作 | 描述 | 示例 |
| ----------- | ---- | ---- |
| [$expand][expand] | 数组展开成多条记录 | db.sample.employee.find( { a: { $expand: 1 } } ) |
| [$returnMatch][returnMatch] | 返回匹配的数组元素 | db.sample.employee.find( { a: { $returnMatch: 0, $in: [ 1, 4, 7 ] } } ) |


[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Function_Operator/Readme.md
[gt]:manual/Manual/Operator/Match_Operator/gt.md
[gte]:manual/Manual/Operator/Match_Operator/gte.md
[lt]:manual/Manual/Operator/Match_Operator/lt.md
[lte]:manual/Manual/Operator/Match_Operator/lte.md
[lte]:manual/Manual/Operator/Match_Operator/lte.md
[ne]:manual/Manual/Operator/Match_Operator/ne.md
[in]:manual/Manual/Operator/Match_Operator/in.md
[nin]:manual/Manual/Operator/Match_Operator/nin.md
[all]:manual/Manual/Operator/Match_Operator/all.md
[and]:manual/Manual/Operator/Match_Operator/and.md
[not]:manual/Manual/Operator/Match_Operator/not.md
[or]:manual/Manual/Operator/Match_Operator/or.md
[exists]:manual/Manual/Operator/Match_Operator/exists.md
[elemMatch]:manual/Manual/Operator/Match_Operator/elemMatch.md
[identifier]:manual/Manual/Operator/Match_Operator/identifier.md
[regex]:manual/Manual/Operator/Match_Operator/regex.md
[mod]:manual/Manual/Operator/Match_Operator/mod.md
[et]:manual/Manual/Operator/Match_Operator/et.md
[isnull]:manual/Manual/Operator/Match_Operator/isnull.md
[expand]:manual/Manual/Operator/Match_Operator/expand.md
[returnMatch]:manual/Manual/Operator/Match_Operator/returnMatch.md
