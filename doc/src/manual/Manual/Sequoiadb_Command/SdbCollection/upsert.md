##名称##

upsert - 更新集合记录

##语法##

**db.collectionspace.collection.upsert\(\<rule\>, \[cond\], \[hint\], \[setOnInsert\], \[options\]\)**

##类别##

SdbCollection

##描述##

该函数用于更新集合记录。upsert 方法跟 update 方法都是对记录进行更新，不同的是当使用 cond 参数在集合中匹配不到记录时，update 不做任何操作，而 upsert 方法会做一次插入操作。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| rule | object | 更新规则，记录按 rule 的内容更新 | 是 |
| cond | object | 选择条件，为空时，更新所有记录，不为空时，更新符合条件的记录 | 否 |
| hint | object | 指定访问计划 | 否 |
| setOnInsert | object | 在做插入操作时向插入的记录中追加字段 | 否 |
| options | object| 可选项，详见 options 选项说明| 否 |

options 选项：

| 参数名 | 类型 | 描述 | 默认值 |
| ------ | ---- | ---- | -------- |
| KeepShardingKey | boolean     | 为 false 时，将不保留更新规则中的分区键字段，只更新非分区键字段<br>为 true 时，将会保留更新规则中的分区键字段| false  |
| JustOne         | boolean     | 为 true 时，将只更新一条符合条件的记录<br>为 false 时，将会更新所有符合条件的记录| false  |

> **Note:**
>
> * 参数 hint 的用法与 [find()][find] 的相同。
> * 当 cond 参数在集合中匹配不到记录时，upsert 会生成一条记录插入到集合中。记录生成规则为：首先从 cond 参数中取出 $et 和 $all 操作符对应的键值对，没有的时候生成空记录。然后使用 rule 规则对其做更新操作，最后加入 setOnInsert 中的键值对。
> * 目前分区集合上，不支持更新分区键。如果 `KeepShardingKey` 为 true，并且更新规则中带有分区键字段，将会报错 -178。
> * JustOne 为 true 时，只能在单个分区、单个子表上执行。

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取成功更新的记录数信息，字段说明如下：

| 字段名 | 类型 | 描述 |
|--------|------|------|
| UpdatedNum | int64 | 成功更新的记录数，包括匹配但未发生数据变化的记录 |
| ModifiedNum | int64 | 成功更新且发生数据变化的记录数 |
| InsertedNum | int64 | 成功插入的记录数 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

`upsert()` 函数常见异常如下：
  
| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -178     | SDB_UPDATE_SHARD_KEY|分区集合上不支持更新分区键| KeepShardingKey 设置为 false，不更新分区键   |
| -347     |SDB_COORD_UPDATE_MULTI_NODES|参数 JustOne 为 true 时，跨多个分区或多个子表更新记录 | 修改匹配条件或不使用参数 JustOne |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

假设集合 sample.employee 中有两条记录

```lang-json
{
  "_id": {
    "$oid": "516a76a1c9565daf06030000"
  },
  "age": 10,
  "name": "Tom"
}
{
  "_id": {
    "$oid": "516a76a1c9565daf06050000"
  },
  "a": 10,
  "age": 21
}
```

* 按指定的更新规则更新集合中所有记录，即设置 rule 参数，不设定 cond 和 hint 参数的内容。如下操作等效于使用 update 方法，更新集合 sample.employee 中的所有记录，使用 [$inc][inc] 将记录的 age 字段值加1，name 字段值更改为“Mike”，对不存在 name 字段的记录，[$set][set] 操作符会将 name 字段和其设定的值插入到记录中，可使用 find 方法查看更新结果

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 1}, $set: {name: "Mike"}})
    {
      "UpdatedNum": 2,
      "ModifiedNum": 2,
      "InsertedNum": 0
    }
    >
    > db.sample.employee.find()
    {
         "_id": {
         "$oid": "516a76a1c9565daf06030000"
         },
         "age": 11,
         "name": "Mike"
    }
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "a": 10,
         "age": 22,
         "name":"Mike"
    }
    Return 2 row(s).
    ```

* 选择符合匹配条件的记录，对这些记录按更新规则更新，即设定 rule 和 cond 参数。如下操作使用 [$exists][exists] 匹配存在 type 字段的记录，使用 [$inc][inc] 将这些记录的 age 字段值加 3。在上面给出的两条记录中，都没有 type 字段，此时，upsert 操作会插入一条新的记录，新记录只有 \_id 字段和 age 字段名，\_id 字段值自动生成，而 age 字段值为 3

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 3}}, {type: {$exists: 1}})
    {
      "UpdatedNum": 0,
      "ModifiedNum": 0,
      "InsertedNum": 1
    }
    >
    > db.sample.employee.find()
    {
         "_id": {
         "$oid": "516a76a1c9565daf06030000"
         },
         "age": 11,
         "name": "Mike"
    }
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "a": 10,
         "age": 22,
         "name":"Mike"
    }
    {
         "_id": {
         "$oid": "516cfc334630a7f338c169b0"
         },
         "age": 3
    }
    Return 3 row(s).
    ```

* 按访问计划更新记录，假设集合中存在指定的索引名 testIndex，此操作等效于使用 update 方法，使用索引名为 testIndex 的索引访问集合 sample.employee 中 age 字段值大于 20 的记录，将这些记录的 age 字段名加 1

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 1}}, {age: {$gt: 20}}, {"": "testIndex"})
    {
      "UpdatedNum": 1,
      "ModifiedNum": 1,
      "InsertedNum": 0
    }
    >
    > db.sample.employee.find()
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "a": 10,
         "age": 23,
         "name":"Mike"
    }
    Return 1 row(s).
    ```

* 使用 setOnInsert 更新记录，由于集合 sample.employee 中 age 字段值大于 30 的记录为空，upsert 在做插入操作时向插入的记录中追加字段 {"name":"Mike"}

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 1}}, {age: {$gt: 30}}, {}, {"name": "Mike"})
    {
      "UpdatedNum": 0,
      "ModifiedNum": 0,
      "InsertedNum": 1
    }
    >
    > db.sample.employee.find({"age": 1, "name": "Mike"})
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "age":1,
         "name":"Mike"
    } 
    Return 1 row(s).
    ```

* 分区集合 sample.employee，分区键为 {a: 1}，含有以下记录

    ```lang-javascript
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5c6f660ce700db6048677154"
      },
      "a": 1,
      "b": 1
    }
    Return 1 row(s).
    ```
 
    指定 KeepShardingKey 参数：不保留更新规则中的分区键字段，只更新了非分区键 b 字段，分区键 a 字段的值没有被更新。
 
    ```lang-javascript
    > db.sample.employee.upsert({$set: {a: 9, b: 9}}, {}, {}, {}, {KeepShardingKey: false})
    {
      "UpdatedNum": 1,
      "ModifiedNum": 1,
      "InsertedNum": 0
    }
    >
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5c6f660ce700db6048677154"
      },
      "a": 1,
      "b": 9
    }
    Return 1 row(s).
    ```
 
    指定 KeepShardingKey 参数：保留更新规则中的分区键字段。因为目前不支持更新分区键，所以会报错。
 
    ```lang-javascript
    > db.sample.employee.upsert({$set: {a: 9}}, {}, {}, {}, {KeepShardingKey: true})
    (nofile):0 uncaught exception: -178
    Sharding key cannot be updated
    ```


[^_^]:
     本文使用的所有引用及链接
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[inc]:manual/Manual/Operator/Update_Operator/inc.md
[exists]:manual/Manual/Operator/Match_Operator/exists.md
[set]:manual/Manual/Operator/Update_Operator/set.md