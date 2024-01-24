[^_^]:
    数据分区操作实例
    作者：林友滨
    时间：20190320
    评审意见
    林友滨：初稿完成；时间：20190320
    市场部：时间：20190325


为了进一步帮助用户更好地理解和使用数据分区，本文档通过以下操作示例进行解释说明。以下示例均使用 SDB Shell 进行操作。

数据库分区
----

### 使用范围分区方式做数据库分区 ###

以下示例是在集合 sample.employee 中，将 create_date 字段范围在[201801, 201901)的数据切分到复制组 group2 上，其它数据切分到复制组 group1 上。

1. [创建集合][create_collection] sample.employee，分区键为 create_date 字段，分区方式为范围分区（range），集合所在复制组为 group1 

   ```lang-javascript
   > db.createCS( "sample" )
   > db.sample.createCL( "employee", { ShardingKey: { create_date: 1 }, ShardingType: "range", Group: "group1" } )
  ```

2. 执行[切分操作][split]，将集合 sample.employee 中 create_date 字段数据范围在[201801, 201901)的记录，从复制组 group1 切分到复制组 group2

   ```lang-javascript
   > db.sample.employee.split( "group1", "group2", { create_date: "201801"}, { create_date: "201901"} )
   ```

操作成功后，通过集合 sample.employee 插入 2018 年范围内的记录就会写到复制组 group2 上，插入其它年份的记录就会写到复制组 group1 上。

### 使用散列分区方式做数据库分区 ###

以下示例是在集合 sample.employee2 中，以记录 id 字段为分区键进行散列分区，将 hash 值范围在[2048, 4096)的数据切分到复制组 group2 上，其它数据切分到复制组 group1 上。

1. 创建集合 sample.employee2，分区键为 id 字段，分区方式为散列分区（hash），hash 值总数为 4096 个，集合所在复制组为 group1 

   ```lang-javascript
   > db.sample.createCL( "employee2", { ShardingKey: { id: 1 }, ShardingType: "hash", Partition: 4096, Group: "group1" } )
   ```

2. 执行切分命令，将集合 sample.employee2 中 id 字段 hash 值范围在[2048, 4096)的记录，从复制组 group1 切分到复制组 group2 

   ```lang-javascript
   > db.sample.employee2.split( "group1", "group2", { Partition: 2048 }, { Partition: 4096 } )
   ```

表分区
----

### 使用范围分区方式做表分区 ###

以下示例是在集合 maincs.maincl 中，将 create_date 字段范围在[201801, 201901)中的数据映射到集合 bill.year2018，范围在[201901, 202001)中的数据映射到集合 bill.year2019。

1. 创建主集合 maincs.maincl，分区键为 create_date，分区方式为范围分区

   ```lang-javascript
   > db.createCS( "maincs" )
   > db.maincs.createCL( "maincl", { IsMainCL: true, ShardingKey: { create_date: 1 }, ShardingType: "range" } )
   ```

2. 创建子集合 bill.year2018 和 bill.year2019

   ```lang-javascript
   > db.createCS( "bill" )
   > db.bill.createCL( "year2018" )
   > db.bill.createCL( "year2019" )
   ```
> **Note：**
> 
> 如果需要对子集合 bill.year2018 和 bill.year2019 做数据库分区，可参考数据库分区示例

3. 通过[挂载操作][attach_command]，将主集合 maincs.maincl 和两个子集合进行关联

   ```lang-javascript
   > db.maincs.maincl.attachCL( "bill.year2018", { LowBound: { create_date: "201801" }, UpBound: { create_date: "201901" } } )
   > db.maincs.maincl.attachCL( "bill.year2019", { LowBound: { create_date: "201901" }, UpBound: { create_date: "202001" } } )
   ```

操作成功后，通过主集合 maincs.maincl 插入 2018 年范围内的记录就会写到子集合 bill.year2018 上，插入 2019 年范围内的记录就会写到子集合 bill.year2019 上。

[^_^]: 
    本文使用到的所有链接及引用。
[create_collection]:manual/Manual/Sequoiadb_Command/Sdb/createCS.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md
[attach_command]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md