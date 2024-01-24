##名称##

aggregate - 计算集合中数据的聚合值

##语法##

**db.collectionspace.collection.aggregate( \<subOp1\>，[subOp2]，... )**

##类别##

SdbCollection

##描述##

该函数用于计算集合中数据的聚合值。

##参数##

* `subOp`( *Object*， *必填* )

    subOp1，subOp2... 表示包含[聚集符](manual/Manual/Operator/Aggregate_Operator/Readme.md)的子操作，在 aggregate() 方法中可以填写 1~N 个子操作。每个子操作是一个包含聚集符的 Object 对象，子操作之间用逗号隔开。注意各子操作的参数名的语法规则。

    >**Note:**
    >
    > aggregate 方法会根据子操作的顺序从左到右依次执行每个子操作。

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`aggregate()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确。|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v1.0 及以上版本

##示例##

假设集合 collection 包含如下格式的记录：

```
{
  no:1000,
  score:80,
  interest:["basketball","football"],
  major:"计算机科学与技术",
  dep:"计算机学院",
  info:
  {
    name:"Tom",
    age:25,
    gender:"男"
  }
}
```

- 按条件选择记录，并指定返回字段名，如下聚集操作操作首先使用 $match 选择
   匹配条件的记录，然后使用 $project 只返回指定的字段名。

    ```lang-javascript
    > db.sample.employee.aggregate( { $match: { $and: [ { no: { $gt: 1002 } },
                                                    { no: { $lt: 1015 } },
                                                    { dep: "计算机学院" } ] } },
                                  { $project: { no: 1, "info.name": 1, major: 1 } } )
    {
        "no": 1003,
        "info.name": "Sam",
        "major": "计算机软件与理论"
    }
    {
        "no": 1004,
        "info.name": "Coll",
        "major": "计算机工程"
    }
    {
        "no": 1005,
        "info.name": "Jim",
        "major": "计算机工程"
    }
    ```
- 按条件选择记录，并对记录进行分组。如下操作首先使用 $match 选择匹配条件的记录，
   然后使用 $group 对记录按字段 major 进行分组，并使用 $avg 返回每个分组中嵌套
   对象 age 字段的平均值。

    ```lang-javascript
    > db.sample.employee.aggregate( { $match: { dep:  "计算机学院" } },
                                  { $group: { _id:  "$major", Major: { $first: "$major" },
                                  avg_age: { $avg: "$info.age" } } } )
   {
    "Major": "计算机工程",
    "avg_age": 25
    }
    {
       "Major": "计算机科学与技术",
       "avg_age": 22.5
    }
    {
       "Major": "计算机软件与理论",
       "avg_age": 26
    }
    ```

- 按条件选择记录，并对记录进行分组、排序、限制返回记录的起始位置和返回记录数。
   如下操作首先按 $match 选择匹配条件的记录；然后使用 $group 按 major 进行分组，
   并使用 $avg 返回每个分组中嵌套对象 age 字段的平均值，输出字段名为 avg_age；
   最后使用 $sort 按 avg_age 字段值（降序），major 字段值（降序）对结果集进行
   排序，使用 $skip 确定返回记录的起始位置，使用 $limit 限制返回记录的条数。

    ```lang-javascript
    > db.sample.employee.aggregate( { $match: { interest: { $exists: 1 } } },
                                  { $group: { _id: "$major",
                                              avg_age: { $avg: "$info.age" },
                                              major: { $first: "$major" } } },
                                  { $sort: { avg_age: -1, major: -1 } },
                                  { $skip: 2 },
                                  { $limit: 3 } )
    {
       "avg_age": 25,
        "major": "计算机科学与技术"
    }
    {
        "avg_age": 22,
        "major": "计算机软件与理论"
    }
    {
        "avg_age": 22,
        "major": "物理学"
    }
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
