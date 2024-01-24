[^_^]:
    多维分区
    作者：林友滨
    时间：20190416
    评审意见
    林友滨：初稿完成；时间：20190510
    市场部：时间：20190528


用户可以使用表分区或数据库分区提高数据访问性能，但在数据量快速增长的场景下性能会逐渐下降，多维分区可以解决这一问题。本文档将介绍多维分区的实现原理和操作实例。

##原理介绍##

多维分区主要用于处理既要减少数据访问量，又要提高数据并行计算能力的场景。多维分区示意图：

![多维分区示意图][intro_sharding_multiple]

+ 对主集合进行表分区，将多个分区映射到不同的子集合后，再针对某一个子集合使用数据库分区，将子集合中的数据切分到不同的数据组中
+ 当需要访问某一范围内的数据时，既可以将数据访问集中在若干个子集合中，又能同时发挥不同复制组并行计算的能力，从而提高处理速度和性能

##业务场景举例##

下面以银行业务账单为例，简单介绍一下多维分区的作用。

+ 账单数据具有很强的时间特性，比如查询某月的账单。针对这一特性可以将时间作为分区键，先对主集合进行表分区，将一个月的数据映射到一个子集合上。针对子集合（一个月内的数据），以帐号 id 再进行一次数据库分区，将数据映射到多个数据组上。
+ 当需要查询某个月的账单时，数据库首先会集中到某一个子集合上去查询，而不会访问其它集合的数据，访问的数据量大大减少；而由于子集合做了数据库分区，查询又可以在多个数据组中并行计算，从而提高处理性能。

##多维分区操作示例##

多维分区在操作上，可以先对子集合做数据库分区，然后再通过表分区将子集合挂载到主集合上。

1. 创建主集合 main.bill ，分区键为 bill_date ，分区方式为范围分区（range）

   ```lang-javascript
   > db.createCS("main")
   > db.main.createCL("bill", {IsMainCL: true, ShardingKey: {bill_date: 1}, ShardingType: "range"})
   ```

2. 创建子集合 bill.date_201905 ，分区键为帐号 id 字段，分区方式为散列分区（hash），hash 值总数为 4096 个, 集合所在复制组为 group1

   ```lang-javascript
   > db.createCS("bill")
   > db.bill.createCL("date_201905", {ShardingKey: {id: 1}, ShardingType: "hash", Partition: 4096, Group: "group1"})
   ```

3. 执行切分命令，将集合 bill.date_201905 中，帐号 id 字段 hash 值范围在[2048, 4096)的记录，从复制组 group1 切分到复制组 group2 

   ```lang-javascript
   > db.bill.date_201905.split("group1", "group2", {Partition: 2048}, {Partition: 4096})
   ```

4. 通过[挂载操作][attach_command]，将主集合 main.bill 和子集合 bill.date_201905 进行关联

   ```lang-javascript
   > db.main.bill.attachCL("bill.date_201905", {LowBound: {bill_date: "201905"}, UpBound: {bill_date: "201906"}})
   ```

[^_^]:
    本文使用到的所有链接及引用。
[intro_sharding_multiple]:images/Distributed_Engine/Architecture/Sharding/intro_sharding_multiple.png
[attach_command]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md