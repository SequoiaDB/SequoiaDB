[^_^]:
    SparkSQL 实例-使用


本文档将介绍 Spark-SequoiaDB 的使用。

##配置 SparkSQL 参数##

用户可通过配置文件、命令行及创建语句中的映射表参数配置 SparkSQL。当使用配置文件时，所填配置将对全局生效；当使用命令行时，所填配置仅对当前会话生效；当使用映射表参数时，所填配置仅对当前的表生效，且配置无法修改。因此在创建映射表之前，建议用户将无需与表强关联的配置项，通过配置文件或命令行的方式写入，便于后续修改。

>**Note:**
>
> 配置优先级说明：映射表参数 > 命令行 > 配置文件，优先级高的参数配置将覆盖优先级低的参数配置。<br>
> 为避免与 SparkSQL 中其他参数冲突，填入配置文件或命令行的参数需以 `spark.sequoiadb.config.defaults.` 为前缀。具体参数说明可参考[参数列表][option]。 

###配置文件###

用户通过配置文件更新配置时，新配置对已启动的 Spark Application（如 spark-sql、thrift-server）无效，需重启 Spark Application 以加载新配置。

**示例**

通过 Spark 安装目录下的配置文件 `conf/spark-defaults.conf` 更新配置

```lang-text
spark.sequoiadb.config.defaults.host       sdbServer1:11810,sdbServer2:11810,sdbServer3:11810
spark.sequoiadb.config.defaults.username   sdbadmin
spark.sequoiadb.config.defaults.password   sdbadmin
```

###命令行###

用户通过命令行更新配置时，其优先级高于配置文件参数，即在当前 Spark Application 中覆盖配置文件提供的全局配置参数，新配置仅在对应的 Spark Application 中生效。

**示例**

通过 `spark-sql/beeline` 更新配置

```lang-sql
spark-sql> SET spark.sequoiadb.config.defaults.connecttime=4000;
```

##创建映射表##

###语法###

SparkSQL 创建 SequoiaDB 表语句的格式如下：

```lang-sql
CREATE <[temporary] TABLE| temporary VIEW> <tableName> [(schema)] USING com.sequoiadb.spark OPTIONS (<option>, <option>, ...)
```

- temporary：临时表或视图，只在创建表或视图的会话中有效，会话退出后自动删除。

- schema：可不填，连接器会自动生成。自动生成的 schema 字段顺序与集合中记录的顺序不一致，因此如果对 schema 的字段顺序有要求，应该显式定义 schema 。

- option：[参数列表][option]，参数是键和值都为字符串类型的键值对，其中值的前后需要有单引号，多个参数之间用逗号分隔。

> **Note:**
>
> 通过 `OPTIONS` 子句可指定映射表参数，其优先级高于配置文件参数和会话参数，且无需指定前缀 `spark.sequoiadb.config.defaults`。

###示例###

1. 假设集合名为 test.data ，协调节点在 sdbserver1 和 sdbserver2 上，通过 spark-sql 创建一个表来对应 SequoiaDB 的集合

   ```lang-sql
   spark-sql> create table datatable(c1 string, c2 int, c3 int) using com.sequoiadb.spark options(host 'sdbserver1:11810,sdbserver2:11810', collectionspace 'test', collection 'data');
   ```

2. 从 SequoiaDB 的表 t1 向表 t2 插入数据

   ```lang-sql
   spark-sql> insert into table t2 select * from t1;
   ```

##参数列表##

| 名称     | 类型      | 默认值  | 描述 | 是否必填|
| ---------| --------- | -------- |-------|--------|
|host|string|-|SequoiaDB 协调节点地址，多个地址以","分隔，例如："server1:11810,server2:11810"|是|
|collectionspace|string|-|集合空间名称|是|
|collection|string|-|集合名称（不包含集合空间名称）|是|
|username|string|""|用户名|否|
|passwordtype|string|"cleartext"|密码类型，取值如下：<br>"cleartext"：表示参数 password 为明文密码<br>"file"：表示参数 password 为密码文件路径|否|
|password|string|""|用户名对应的密码|否|
|connecttimeout|int32|1000|连接 SequoiaDB 节点的超时时间（单位：ms）<br>取值为 0 表示不进行超时检测 |否|
|samplingratio|double|1|schema 采样率，取值范围为(0, 1.0]|否|
|samplingnum|int64|1000|schema 采样数量（每个分区），取值大于 0|否|
|samplingwithid|boolean|FALSE|schema 采样时是否带 _id 字段，取值为 true 或 false  |否|
|samplingsingle|boolean|TRUE|schema 采样时使用一个分区，取值为 true 或 false |否|
|bulksize|int32|500|向 SequoiaDB 集合插入数据时批插的数据量，取值大于 0 |否|
|partitionmode|string|auto|分区模式，默认值为"auto"，取值如下：<br>"auto"：自动选择模式 <br>"sharding"：以分区为单位进行并发读取 <br> "datablock"：以集合为单位进行并发读取 <br> 该参数取值为"auto"时，如果查询使用了索引则自动选择"sharding"模式，未使用索引则选择"datablock"模式  |否|
|partitionblocknum|int32|4|每个分区的数据块数，在按 datablock 分区时有效，取值大于 0 |否|
|partitionmaxnum|int32|1000|最大分区数量，在按 datablock 分区时有效，取值大于等于 0，等于 0 时表示不限制分区最大数量<br>由于 partitionMaxNum 的限制，每个分区的数据块数可能与 partitionBlockNum 不同 |否|
|preferredinstance|string|"A"|指定分区优先选择的节点实例，取值可参考 [preferredinstance][parameter]|否|
|preferredinstancemode|string|"random"|在 preferredinstance 有多个实例符合时的选择模式，取值可参考 [preferredinstancemode][parameter]|否|
|preferredinstancestrict|boolean|TRUE|在 preferredinstance 指定的实例 ID 都不符合时是否报错 |否|
|ignoreduplicatekey|boolean|FALSE|向表中插入数据时忽略主键重复的错误 |否|
|ignorenullfield|boolean|FALSE|向表中插入数据时忽略值为 null 的字段 |否|
|pagesize|int32|65536|create table as select 创建集合空间时指定数据页大小，如果集合空间已存在则忽略该参数 |否|
|domain|string|-|create table as select 创建集合空间时指定所属域，如果集合空间已存在则忽略该参数 |否|
|shardingkey|json|-|create table as select 创建集合时指定分区键 |否|
|shardingtype|string|"hash"|create table as select 创建集合时指定分区类型，取值可以是"hash"和"range" |否|
|replsize|int32|1|create table as select 创建集合时指定副本写入数 |否|
|compressiontype|string|"none"|create table as select 创建集合时指定压缩类型，取值可以是"none"、"lzw"和"snappy"，"none"表示不压缩 |否|
|autosplit|boolean|FALSE|create table as select 创建集合时指定是否自动切分，必须配合散列分区和域使用，且不能与 group 同时使用 |否|
|group|string|-|create table as select 创建集合时指定创建在某个复制组，group 必须存在于集合空间所属的域中 |否|
|ensureshardingindex|boolean|TRUE|create table as select 创建集合时指定是否使用 ShardingKey 包含的字段创建名字为"$shard"的索引 |否|
|autoindexid|boolean|TRUE|create table as select 创建集合时指定是否自动使用字段 _id 创建名字为"$id"的唯一索引 |否|
|strictdatamode|boolean|FALSE|create table as select 创建集合时指定对该集合的操作是否开启严格数据模式 <br>开启严格数据模式后对数值操作将存在以下限制：<br>1）运算过程不改数据类型<br>2）数值运算出现溢出时直接报错|否|
|autoincrement|json|-|create table as select 创建集合时指定集合使用的自增字段<br>自增字段相关说明可参考 [autoincrement][autoincrement] |否|


[^_^]:
     本文使用的所有引用和链接
[parameter]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[autoincrement]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[option]:manual/Database_Instance/Json_Instance/Development/c_driver/usage.md#参数列表