
编目信息快照可以列出当前数据库中所有集合的编目信息，每个集合一条记录。

>   **Note:**
>
>   该快照只能在协调节点执行。

##标识##

$SNAPSHOT_CATA

##字段信息##

| 字段名              | 类型   | 描述                         |
| ------------------- | ------ | ---------------------------- |
| Name                | string | 集合完整名                   |
| UniqueID            | int64  | 集合的 UniqueID，在集群上全局唯一 |
| EnsureShardingIndex | boolean | 是否自动为分区键字段创建索引 |
| ReplSize            | int32   | 执行修改操作时需要同步的副本数<br>当执行更新、插入、删除记录等操作时，仅当指定副本数的节点都完成操作时才返回操作结果 |
| ConsistencyStrategy | int32   | 集合的[同步一致性][ConsistencyStrategy]策略 |
| ShardingKey         | object | 数据分区类型，取值如下：<br> "range"：数据按分区键值的范围进行分区存储<br>"hash"：数据按分区键的哈希值进行分区存储 |
| Version             | int32  | 集合版本号，当对集合的元数据执行修改操作时递增该版本号（例如数据切分） |
| Attribute           | int32  | 集合的属性掩码，取值可参考 [SYSCOLLECTION 集合][syscollection]                     |
| AttributeDesc       | string | 集合的属性描述，取值可参考 [SYSCOLLECTION 集合][syscollection]                |
| CompressionType     | int32  | 压缩算法类型，取值可参考 [SYSCOLLECTION 集合][syscollection]                 |
| CompressionTypeDesc | string | 压缩算法类型描述，取值可参考 [SYSCOLLECTION 集合][syscollection]            |
| Partition           |int32  | hash 分区的个数 ( 仅数据库分区集合显示 )|
| InternalV           | int32  | hash 算法版本号 ( 仅数据库分区集合显示，内部使用 )      |
| AutoSplit           | boolean | 集合是否开启自动切分功能 ( 仅数据库分区集合显示 )      |
| IsMainCL            | boolean  | 集合是否为表分区中的主表 ( 仅表分区集合显示 )    |
| MainCLName          | string | 集合在表分区中所关联的主表名 ( 仅表分区集合显示 )|
| CataInfo.ID         | int32  | 子表挂载的顺序 ID ( 内部使用 ) |
| CataInfo.SubCLName  | string | 子表名 ( 仅表分区集合显示 )  |
| CataInfo.GroupID    | int32  | 复制组 ID                    |
| CataInfo.GroupName  | string | 复制组名                     |
| CataInfo.LowBound   | object | 数据分区区间的上限           |
| CataInfo.UpBound    | object | 数据分区区间的下限           |
| AutoIncrement.Field | string | 自增字段名称                 |
| AutoIncrement.Generated | string | 自增字段生成方式         |
| AutoIncrement.SequenceName | string | 自增字段对应序列名    |
| AutoIncrement.SequenceID | int64  | 自增字段对应序列ID      |
| DataSourceID      | int32  | 数据源 ID                      |
| Mapping           | string | 在[数据源][datasource]中所映射的集合名称    |
| CreateTime | string | 创建集合的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合元数据的时间（仅在 v3.6.1 及以上版本生效） |

##示例##

- 查看普通集合的编目信息快照

   ```lang-javascript
   > db.exec("select * from $SNAPSHOT_CATA")
   ```

   输出信息如下：

   ```lang-json
   {
     "_id": {
       "$oid": "5e4245f9e86d05a0a03e69c8"
     },
     "Name": "sample.employee",
     "UniqueID": 4294967297,
     "Version": 1,
     "Attribute": 1,
     "AttributeDesc": "Compressed",
     "CompressionType": 1,
     "CompressionTypeDesc": "lzw",
     "CataInfo": [
       {
         "GroupID": 1000,
         "GroupName": "group1"
       }
     ],
     "CreateTime": "2022-10-06-18.04.31.090000",
     "UpdateTime": "2022-10-06-18.04.31.164000"
   }

   ```

- 查看数据库分区集合的编目信息快照

   ```lang-javascript
   > db.exec("select * from $SNAPSHOT_CATA")
   ```

   输出结果如下：

   ```lang-json
   {
     "_id": {
       "$oid": "5247a2bc60080822db1cfba2"
     },
     "Name": "sample.employee",
     "UniqueID": 261993005057,
     "Version": 1,
     "Attribute": 0,
     "AttributeDesc": "",
     "AutoIncrement": [
       {
         "SequenceName": "SYS_261993005057_studentID_SEQ",
         "Field": "studentID",
         "Generated": "default",
         "SequenceID": 4
       }
     ],
     "CompressionType": 0,
     "CompressionTypeDesc": "snappy",
     "ConsistencyStrategy": 3,
     "ReplSize": 1,
     "ShardingKey": {
       "age": 1
     },
     "EnsureShardingIndex": true,
     "ShardingType": "hash",
     "Partition": 4096,
     "InternalV": 3,
     "CataInfo": [
       {
         "ID": 0,
         "GroupID": 1000,
         "GroupName": "group1",
         "LowBound": {
           "": {
             "$minKey": 1
           }
         },
         "UpBound": {
           "": {
             "$maxKey": 1
           }
         }
       }
     ]
     "AutoSplit": ture,
     "CreateTime": "2022-10-06-18.04.31.008000",
     "UpdateTime": "2022-10-06-18.05.49.384000"
   }
   ```

- 查看表分区集合的编目信息快照

   ```lang-javascript
   > db.exec("select * from $SNAPSHOT_CATA")
   ```

   输出结果如下：

   ```lang-json
   {
     "_id": {
       "$oid": "5e426b88e86d05a0a03e69c9"
     }
     "Name": "year_2019.month",
     "UniqueID": 4294967298,
     "Attribute": 1,
     "AttributeDesc": "Compressed",
     "CataInfo": [
       {
         "ID": 1,
         "SubCLName": "year_2019.month_07",
         "LowBound": {
           "date": "20190701"
         },
         "UpBound": {
           "date": "20190801"
         }
       }
     ],
     "CompressionType": 1,
     "CompressionTypeDesc": "lzw",
     "EnsureShardingIndex": true,
     "IsMainCL": true,
     "LobShardingKeyFormat": "YYYYMMDD",
     "ShardingKey": {
       "date": 1
     },
     "ShardingType": "range",
     "Version": 2,
     "CreateTime": "2022-10-06-18.04.31.008000",
     "UpdateTime": "2022-10-06-18.05.49.384000"
   }
   ```

- 查看使用数据源的集合对应的编目信息快照

    ```lang-javascript
    > db.snapshot("select * from $SNAPSHOT_CATA")
    ```

    输出结果如下：

    ```lang-json
    {
      "_id": {
        "$oid": "5ffc313972e60c4d9be30c4f"
      },
      "Name": "sample2.employee",
      "UniqueID": 8589934593,
      "Version": 1,
      "Attribute": 1,
      "AttributeDesc": "Compressed",
      "CompressionType": 1,
      "CompressionTypeDesc": "lzw",
      "CataInfo": [
        {
          "GroupID": -2147483647,
          "GroupName": "DataSource"
        }
      ],
      "DataSourceID": 1,
      "Mapping": "sample2.employee"
    }
    ```


[^_^]:
    本文使用的所有引用及链接
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[syscollection]:manual/Manual/Catalog_Table/SYSCOLLECTIONS.md
[ConsistencyStrategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md