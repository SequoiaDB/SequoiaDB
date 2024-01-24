SequoiaDB 巨杉数据库支持以下聚集符：


| 参数名 | 描述 | 示例 |
| ------ | ---- | ---- |
| [$project][project] | 选择需要输出的字段名，1 表示输出，0 表示不输出，还可以实现字段的重命名 |{ $project: { field1: 1, field: 0, aliase: "$field3" } } |
| [$match][match] | 实现从集合中选择匹配条件的记录，相当于 SQL 语句的 where | {$match: { field: { $lte: value } } } |
| [$limit][limit] | 限制返回的记录条数 | { $limit: 10 } |
| [$skip][skip] | 控制结果集的开始点，即跳过结果集中指定条数的记录 | { $skip: 5 } |
| [$group][group] | 实现对记录的分组，类似于 SQL 的 group by 语句，_id 指定分组字段 | { $group: { _id: "$field" } } |
| [$sort][sort] | 实现对结果集的排序，1 代表升序，-1 代表降序 | { $sort: { field1: 1, field2: -1, ... } } |

>  **Note:**
>
>  聚集符的使用方法可以参考 [SdbCollection.aggregate\(\)][aggregate]。


[^_^]:
    本文使用的所有引用及链接
[project]:manual/Manual/Operator/Aggregate_Operator/project.md
[match]:manual/Manual/Operator/Aggregate_Operator/match.md
[limit]:manual/Manual/Operator/Aggregate_Operator/limit.md
[skip]:manual/Manual/Operator/Aggregate_Operator/skip.md
[group]:manual/Manual/Operator/Aggregate_Operator/group.md
[sort]:manual/Manual/Operator/Aggregate_Operator/sort.md
[aggregate]:manual/Manual/Sequoiadb_Command/SdbCollection/aggregate.md