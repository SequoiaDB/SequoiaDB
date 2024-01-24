[^_^]:
    分区键
    作者：林友滨
    时间：20190524
    评审意见
    林友滨：初稿完成；时间：20190524
    市场部：时间：


在集合中，作为数据分区划分依据的字段称为分区键。在范围分区方式中，分区键是用于划分数据范围的字段；在散列分区中，分区键是用于计算 hash 值的字段。每个分区键可以包含一个或多个字段。

在创建集合时可以指定分区类型和分区键，具体命令可参考[创建集合][createCL]。

范围分区键
----

### 格式 ###

使用范围分区方式进行数据分区时，分区键格式如下：

```lang-json
{
  ShardingKey: { <字段1>: <1|-1>, [<字段2>: <1|-1>, ...] },
  ShardingType: "range"
}
```

- 可以指定多个字段做为分区键
- 对于每个字段可以指定其值为 1 或者 -1 ，表示正向或是逆向排序

### 示例 ###

范围分区方式的分区键一般选择具有序列性的字段，比如时间字段

```lang-javascript
> db.business.createCL( "orders", { ShardingKey: { create_date: 1 }, ShardingType: "range", Group: "group1" } )
```

散列分区键
----

### 格式 ###

使用散列分区方式做数据分区时，分区键格式如下：

```lang-json
{
  ShardingKey: { <字段1>: <1>, [<字段2>: <1>, ...] },
  ShardingType: "hash",
  Partition: <分区数>
}
```

+ 可以指定多个字段做为分区键
+ 散列分区方式中字段没有正向或是逆向的属性
+ Partition 必须是 2 的幂，范围在[ 2^3 , 2^20 ]，默认为 4096

### 示例 ###

散列分区方式的分区键一般选择具有关键属性的字段，比如用户 id 字段

```lang-javascript
> db.business.createCL( "orders2", { ShardingKey: { id: 1 }, ShardingType: "hash", Partition: 4096, Group: "group1" } )
```

查看分区键
----

对于已经建好分区键的集合，可以通过[快照命令][snapshot]查看分区键的定义及分区键划分情况

```lang-javascript
> db.snapshot( SDB_SNAP_CATALOG, { Name: "business.orders" } )
```

输出结果如下：

```lang-json
{
  ...
  "Name": "business.orders",
  "ShardingKey": {
    "create_date": 1
  },
  "ShardingType": "range",
  "CataInfo": [
    {
      "GroupID": 1000,
      "GroupName": "group1",
      "LowBound": {
        "create_date": {
          "$minKey": 1
        }
      },
      "UpBound": {
        "create_date": {
          "$maxKey": 1
        }
      }
    }
  ]
}
```


[^_^]:
    本文使用到的所有链接及引用。
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[snapshot]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md