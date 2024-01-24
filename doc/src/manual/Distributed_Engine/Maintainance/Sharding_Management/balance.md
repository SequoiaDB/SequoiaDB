[^_^]:
    分区数据均衡
    作者：魏彰凯
    时间：20190519
    评审意见
    王涛：
    许建辉：
    市场部：20190705


数据均衡是指不同分区间数据量的均衡。在 SequoiaDB 巨杉数据库中，将集合数据拆分成若干小的数据集进行管理，达到并行计算和减小数据访问量的目的，提升数据库性能。各数据集间数据量的均衡，是数据库性能调优方向之一。本文档将从数据类别、数据域的规划、分区键的选择、数据均衡的判断和数据失衡的处理等方面来介绍分区数据均衡的最佳实践。

数据类别
----

用户可以通过明确数据用途后合理利用分区，提高 SQL 执行效率。根据数据的用途不同，可以将数据分为三类：配置类数据、信息类数据和流水类数据。

- 配置类数据：配置类数据存储系统运行所需的各种配置信息。配置类数据是系统运行的基础，但数据量较少，适合存储在单节点。

- 信息类数据：记录固有信息的数据。该类数据特点为数据量较大，数据操作主要为查询和修改，新增数据一般较少。适合存储在多个节点。例：用户信息数据、资产信息数据。

- 流水类数据：按照时间先后顺序记录每一次交易的数据。该类数据特点为数据量初始较少，增量大。适合存储在方便动态扩容的节点。例：交易流水数据、日志流水数据。

数据域的规划
----

[数据域][sharding_domain]的合理划分可以进行良好的数据分布控制，提升访问效率，优化数据库性能。数据域的规划需要根据系统内集合的职能进行规划，因此可以将数据域规划为以下三类：

- 配置数据域：配置数据域中数据量较少，仅包含一个数据组，存储配置类数据。

- 信息数据域：信息数据域中数据量较多，但增量较少，单集合数据量为数千万条，包含三个数据组到五个数据组，存储信息类数据。

- 流水数据域：流水数据域中数据量很多，增量大，包含其余所有数据组，是以后扩容的重点关注域，存储流水类数据。

分区键的选择
----

[分区键][sharding_key]定义了每个集合中所包含数据的分区规则。每一个集合对应一个分区键，分区键中可以包含一个或多个字段。集合和用途的不同决定了分区键的选择方式有所不同。

> **Note:**
>
> - 信息类数据存储对象的属性数据，以客户号和资产编号等唯一值或者是唯一索引作为分区键。
> - 信息类数据数据量较为固定，仍需要预留一定的存储空间。
> - 流水类数据按照时间不断递增，以发生日期字段作为主集合分区键，流水号或者客户号作为子集合分区键。

数据均衡的判断
----

数据是否均衡，可以从集合空间级别和集合级别这两个级别判断。集合空间级别可以定位集合空间内数据是否失衡并作为参考数据，详细的数据均衡判断需要在集合级别进行。

### 集合空间级别 ###

数据节点的目录下包含了不同集合空间的数据文件，数据文件的大小可以直观反映该集合空间数据量的多少。在不同数据节点下，单个集合空间的数据文件大小差距过大，可以判断为数据失衡。

- 数据均衡示例

   ```lang-bash
   $ ll -h /opt/sequoiadb/database/data/*/test*
   ```
    
   输出结果如下：

   ```lang-text
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11820/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11820/test.1.idx
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11830/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11830/test.1.idx
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11840/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11840/test.1.idx
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11850/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11850/test.1.idx
   ```

- 数据失衡示例

   ```lang-bash
   $ ll -h /opt/sequoiadb/database/data/*/test*
   ```

   输出结果如下：

   ```lang-text
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11820/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11820/test.1.idx
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11830/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11830/test.1.idx
   -rw-r-----. 1 sdbadmin sdbadmin_group 533M Jun  5 02:26 /opt/sequoiadb/database/data/11840/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 273M Jun  5 02:26 /opt/sequoiadb/database/data/11840/test.1.idx
   -rw-r-----. 1 sdbadmin sdbadmin_group 277M Jun  5 02:26 /opt/sequoiadb/database/data/11850/test.1.data
   -rw-r-----. 1 sdbadmin sdbadmin_group 145M Jun  5 02:26 /opt/sequoiadb/database/data/11850/test.1.idx
   ```

> **Note:**  
> - `test.*.data` 文件是集合空间 test 的数据文件
> -  `test.*.idx` 文件是集合空间 test 的索引数据文件
> - 数据失衡示例中，`/opt/sequoiadb/database/data/11840/` 目录下的数据文件明显大于其余目录下的数据文件，因此集合空间 test 中存在数据失衡的集合，需要排查。

### 集合级别 ###

集合级别的数据均衡判断需要根据集合在不同数据节点的数据条数进行判断。通过 [db.snapshot()][db_snapshot] 获取集合快照，根据各数据组的 TotalRecords 值差距来判断数据是否均衡。

- 数据均衡示例

   ```lang-javascript
   > db.snapshot(SDB_SNAP_COLLECTIONS, {Name:"test.balance"})
   ```

   输出结果如下：

   ```lang-json
   {
     "Name": "test.balance",
     "UniqueID": 12884901907,
     "Details": [
       {
         "GroupName": "group1",
         "Group": [
           {
             "ID": 1,
             "LogicalID": 2,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 599207,
             "TotalDataPages": 1245,
             "TotalIndexPages": 297,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 90536,
             "TotalIndexFreeSpace": 2015437,
             "NodeName": "test:11820"
           }
         ]
       },
       {
         "GroupName": "group2",
         "Group": [
           {
             "ID": 0,
             "LogicalID": 1,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 600950,
             "TotalDataPages": 1235,
             "TotalIndexPages": 298,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 97688,
             "TotalIndexFreeSpace": 2030405,
             "NodeName": "test:11830"
           }
         ]
       },
       {
         "GroupName": "group3",
         "Group": [
           {
             "ID": 0,
             "LogicalID": 1,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 599843,
             "TotalDataPages": 1246,
             "TotalIndexPages": 297,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 87764,
             "TotalIndexFreeSpace": 1996993,
             "NodeName": "test:11840"
           }
         ]
       }
     ]
   }
   Return 1 row(s).
   ```

- 数据失衡示例

   ```lang-javascript
   > db.snapshot(SDB_SNAP_COLLECTIONS, {Name:"test.nobalance"})
   ```

   输出结果

   ```lang-json
   {
     "Name": "test.nobalance",
     "UniqueID": 12884901909,
     "Details": [
       {
         "GroupName": "group1",
         "Group": [
           {
             "ID": 0,
             "LogicalID": 5,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 0,
             "TotalDataPages": 0,
             "TotalIndexPages": 2,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 0,
             "TotalIndexFreeSpace": 65515,
             "NodeName": "test:11820"
           }
         ]
       },
       {
         "GroupName": "group2",
         "Group": [
           {
             "ID": 1,
             "LogicalID": 3,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 0,
             "TotalDataPages": 0,
             "TotalIndexPages": 2,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 0,
             "TotalIndexFreeSpace": 65515,
             "NodeName": "test:11830"
           }
         ]
       },
       {
         "GroupName": "group3",
         "Group": [
           {
             "ID": 1,
             "LogicalID": 3,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 1800000,
             "TotalDataPages": 2590,
             "TotalIndexPages": 887,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 74052,
             "TotalIndexFreeSpace": 5846290,
             "NodeName": "test:11840"
           }
         ]
       }
     ]
   }
   Return 1 row(s).
   ```

> **Note:**  
>
> 集合 test.nobalance 的 SDB_SNAP_COLLECTIONS 快照信息中，group3 的 TotalRecords 值远大于其他数据组，表示在 group3 中存储的记录数远大于其他数据组，因此判定为数据失衡。

数据失衡的处理
----

数据失衡时，用户需要确认集合的分区键和现有数据在分区键字段上的值，确认数据失衡原因。同时了解集合整体结构，重新选择分区键。

1. 查看数据失衡集合的分区键

   ```lang-javascript
   > db.snapshot(SDB_SNAP_CATALOG, {Name:"test.nobalance"})
   ```

   输出结果如下：

   ```lang-json
   {
     "_id": {
       "$oid": "5cef6f39c07175a73358f64a"
     },
     "Name": "test.nobalance",
     "UniqueID": 12884901909,
     "Version": 4,
     "ReplSize": -1,
     "Attribute": 1,
     "AttributeDesc": "Compressed",
     "CompressionType": 1,
     "CompressionTypeDesc": "lzw",
     "ShardingKey": {
       "status": 1
     },
     "EnsureShardingIndex": false,
     "ShardingType": "hash",
     "Partition": 4096,
     "InternalV": 3,
     "CataInfo": [
       {
         "ID": 0,
         "GroupID": 1000,
         "GroupName": "group1",
         "LowBound": {
           "": 0
         },
         "UpBound": {
           "": 1365
         }
       },
       {
         "ID": 1,
         "GroupID": 1001,
         "GroupName": "group2",
         "LowBound": {
           "": 1365
         },
         "UpBound": {
           "": 2730
         }
       },
       {
         "ID": 2,
         "GroupID": 1002,
         "GroupName": "group3",
         "LowBound": {
           "": 2730
         },
         "UpBound": {
           "": 4096
         }
       }
     ],
     "AutoSplit": true
   }
   Return 1 row(s).
   ```

2. 集合 test.nobalance 现有分区键为 status，查看现有数据中分区字段的值

   ```lang-javascript
   > db.test.nobalance.find({},{account:null,age:null,status:null}).limit(5)
   ```

   输出结果如下：

   ```lang-json
   {
     "account": D00005,
     "age": 26,
     "status": "正常"
   }
   {
     "account": D00006,
     "age": 27,
     "status": "正常"
   }
   {
     "account": D00008,
     "age": 29,
     "status": "正常"
   }
   {
     "account": D00009,
     "age": 20,
     "status": "正常"
   }
   {
     "account": D00012,
     "age": 20,
     "status": "正常"
   }
   Return 5 row(s).
   ```

   > **Note:** 
   >   
   > 集合的 status 字段都是一样的字符串，不适合做分区键、重新规划分区键和重建集合

3. 创建临时集合 nobalance_temp，以字段 account 为分区键

   ```lang-javascript
   > db.test.createCL("nobalance_temp",{ShardingKey:{account:1},ShardingType:"hash",EnsureShardingIndex:false,AutoSplit:true,Compressed:true,CompressionType:"lzw"});
localhost:11810.test.nobalance_temp
   ```

4. 导出 nobalance 集合中的数据

   ```lang-bash
   $ sdbexprt --hostname localhost --svcname 11810 --type json --dir ./ --cscl test.nobalance
   ```

5. 将导出后的数据导入 nobalance_temp 集合

   ```lang-bash
   $ sdbimprt --hosts=localhost:11810 --type=json -c test -l nobalance_temp --file=test.nobalance.json
   ```

6. 确认数据分布是否均衡

   ```lang-javascript
   > db.snapshot(SDB_SNAP_COLLECTIONS, {Name:"test.nobalance_temp"})
   ```

   输出结果如下：

   ```lang-json
   {
     "Name": "test.nobalance_temp",
     "UniqueID": 12884901911,
     "Details": [
       {
         "GroupName": "group1",
         "Group": [
           {
             "ID": 2,
             "LogicalID": 7,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 599207,
             "TotalDataPages": 1237,
             "TotalIndexPages": 376,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 27324,
             "TotalIndexFreeSpace": 7191122,
             "NodeName": "test:11820"
           }
         ]
       },
       {
         "GroupName": "group2",
         "Group": [
           {
             "ID": 2,
             "LogicalID": 5,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 600950,
             "TotalDataPages": 1250,
             "TotalIndexPages": 352,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 60536,
             "TotalIndexFreeSpace": 5568215,
             "NodeName": "test:11830"
           }
         ]
       },
       {
         "GroupName": "group3",
         "Group": [
           {
             "ID": 2,
             "LogicalID": 5,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 599843,
             "TotalDataPages": 1235,
             "TotalIndexPages": 368,
             "TotalLobPages": 0,
             "TotalDataFreeSpace": 32688,
             "TotalIndexFreeSpace": 6648558,
             "NodeName": "test:11840"
           }
         ]
       }
     ]
   }
   Return 1 row(s).
   ```

7. 数据分布均衡后，集合 nobalance 改名为“nobalance_bak”

   ```lang-javascript
   > db.test.renameCL("nobalance", "nobalance_bak");
   ```

   集合 nobalance_temp 改名为“nobalance” 

   ```lang-javascript
   > db.test.renameCL("nobalance_temp", "nobalance");
   ```

[^_^]:
    本文使用到的所有链接
    
[sharding_domain]:manual/Distributed_Engine/Architecture/domain.md
[sharding_key]:manual/Distributed_Engine/Architecture/Sharding/sharding_keys.md
[db_snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md