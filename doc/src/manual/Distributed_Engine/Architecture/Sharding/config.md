[^_^]:
    分区配置
    作者：林友滨
    时间：20190529
    评审意见
    林友滨：初稿完成；时间：20190529
    市场部：时间：


用户做数据分区时需要进行分区配置，配置的主要内容包括划分每个分区包含的数据范围，以及指定分区的归属。

数据库分区配置
----

假设用户需要在集合 business.orders_2019 中以 id 字段为分区键，将数据均匀切分到两个复制组 prod_part1 和 prod_part2 中。则相应的切分配置如下表：

|分区范围 | 分区所属复制组 | 说明 |
|--------|---------------|------|
|[0, 2048) |prod_part1|id 字段 hash 值范围在 0~2048 范围内的记录切分到复制组 prod_part1 中|
|[2048, 4096) |prod_part2|id 字段 hash 值范围在 2048~4096 范围内的记录切分到复制组 prod_part2 中|

> **Note:**
>
> 对集合 business.orders_2019 进行散列分区，默认情况下 hash 值范围为[0, 4096)


以下根据配置，实际生成集合 business.orders_2019

1. [创建集合][create_collection] business.orders_2019 ，分区键为 id 字段，分区方式为散列分区（hash），集合所在复制组为 prod_part1
   ```lang-javascript
   > db.createCS( "business" )
   > db.business.createCL( "orders_2019", { ShardingKey: { id: 1 }, ShardingType: "hash", Group: "prod_part1" } )
   ```

2. 执行[切分操作][split]，将集合 business.orders_2019 中 id 字段 hash 值范围在[2048, 4096)的记录，从复制组 prod_part1 切分至复制组 prod_part2 

   ```lang-javascript
   > db.business.orders_2019.split( "prod_part1", "prod_part2", { id: 2048}, { id: 4096} )
   ```

3. 通过[快照命令][snapshot]查看分区的划分情况

   ```lang-javascript
   > db.snapshot( SDB_SNAP_CATALOG, { Name: "business.orders_2019" } )
   ```

   输出结果如下：

   ```lang-json
   {
     ...
     "Name": "business.orders_2019",
     "ShardingType": "hash",
     "ShardingKey": {
       "id": 1
     }
     "Partition": 4096,
     "CataInfo": [
       {
         "ID": 0,
         "GroupID": 1000,
         "GroupName": "prod_part1",
         "LowBound": {
           "": 0
         },
         "UpBound": {
           "": 2048
         }
       },
       {
         "ID": 1,
         "GroupID": 1001,
         "GroupName": "prod_part2",
         "LowBound": {
           "": 2048
         },
         "UpBound": {
           "": 4096
         }
       }
     ]
   }
   ```

表分区配置
----

当用户需要将集合 business.orders 以 create_date 字段按年将数据切分到不同的子集合中，相应的切分配置如下表：

|分区范围 | 分区所属子集合 | 说明 |
|--------|---------------|------|
|[201801, 201901) |business.orders_2018|create_date 在 201801 到 201901 范围内的数据切分到子集合 business.orders_2018|
|[201901, 202001) |business.orders_2019|create_date 在 201901 到 202001 范围内的数据切分到子集合 business.orders_2019|

以下根据配置，实际生成集合 business.orders

1. [创建集合][create_collection] business.orders ，指定为主集合，分区键为 create_date 字段，分区方式为范围分区（range）

   ```lang-javascript
   > db.business.createCL( "orders", { IsMainCL: true, ShardingKey: { create_date: 1 }, ShardingType: "range" } )
   ```

2. 通过[挂载操作][attach_command]，将主集合 business.orders 和两个子集合进行关联

   ```lang-javascript
   > db.business.orders.attachCL( "business.orders_2018", { LowBound: { create_date: "201801" }, UpBound: { create_date: "201901" } } )
   > db.business.orders.attachCL( "business.orders_2019", { LowBound: { create_date: "201901" }, UpBound: { create_date: "202001" } } )
   ```
> **Note:**
>
> business.orders_2018 和 business.orders_2019 可以是普通集合，也可以是数据库分区的集合。

3. 用户可以通过[快照命令][snapshot]查看分区的划分情况：

   ```lang-javascript
   db.snapshot( SDB_SNAP_CATALOG, { Name: "business.orders" } )
   ```

   输出结果如下：

   ```lang-json
   {
     ...
     "Name": "business.orders",
     "IsMainCL": true,
     "ShardingType": "range",
     "ShardingKey": {
       "create_date": 1
     }
     "CataInfo": [
       {
         "ID": 1,
         "SubCLName": "business.orders_2018",
         "LowBound": {
           "create_date": "201801"
         },
         "UpBound": {
           "create_date": "201901"
         }
       },
       {
         "ID": 2,
         "SubCLName": "business.orders_2019",
         "LowBound": {
           "create_date": "201901"
         },
         "UpBound": {
           "create_date": "202001"
         }
       }
     ]
   }
   ```

[^_^]:
    本文使用到的所有链接及引用。
[sharding_database]:manual/Distributed_Engine/Architecture/Sharding/architecture.md#数据库分区
[sharding_collection]:manual/Distributed_Engine/Architecture/Sharding/architecture.md#表分区
[create_collection]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md
[attach_command]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md
[snapshot]:manual/Manual/Snapshot/Readme.md