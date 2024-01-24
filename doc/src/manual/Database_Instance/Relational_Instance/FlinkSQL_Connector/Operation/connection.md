 [^_^]:
    FlinkSQL 连接器-连接

Flink 集群启动成功后，用户可通过 FlinkSQL 客户端访问 SequoiaDB 巨杉数据库。

##配置读取并发度##

从 SequoiaDB 读取数据时，用户需在 FlinkSQL 客户端中配置读取并发度，以保证性能。配置读取并发度时，建议取值与 SequoiaDB 集群中复制组的数量成比例。如集群中存在 6 个复制组，建议将读取并发度配置为 6、12 或 24。

1. 切换至 Flink 安装目录，并启动 FlinkSQL 客户端

    ```lang-bash
    $ bin/sql-client.sh
    ```

2. 配置读取并发度为 12

    ```lang-sql
    Flink SQL> SET 'parallelism.default'='12';
    ```

##创建映射表##

Flink 与 SequoiaDB 通过映射表进行数据交互。

1. 切换至 Flink 安装目录，并启动 FlinkSQL 客户端

    ```lang-bash
    $ bin/sql-client.sh
    ```

2. 在 Flink 中创建映射表 employee，映射对象为协调节点 `sdbserver1:11810` 所在集群的集合 sample.employee

   ```lang-sql
   Flink SQL> CREATE TABLE employee (id INT PRIMARY KEY, name STRING, age INT)
   WITH (
    'connector' = 'sequoiadb',
    'hosts' = 'sdbserver1:11810',
    'collectionspace' = 'sample',
    'collection' = 'employee'
    );
    ```
    > **Note:**
    >
    > 如果所映射的集合空间或集合不存在于目标集群中，连接器会在数据写入时自动创建。当映射表存在主键时，连接器会在对应集合中创建名为“primarykey”的唯一索引，该索引的索引键为主键所包含的字段。

##自定义表配置##

Flink 创建映射表语句的格式如下：
   
```lang-sql
CREATE TABLE [IF NOT EXISTS] [catalog_name.][db_name.]table_name (<column1, column2, ...>)
WITH(
'option1'='value1', 
'option2'='value2'
...
);
```

创建成功的映射表，在被使用之前都不会与 SequoiaDB 建立连接，因此不会检测参数 option 所指定配置的正确性。

通过参数 option 可以配置映射表的属性，支持的配置项如下：

| 参数名                  | 类型    | 描述                                                                                                                                                                                                                                                      | 必填 |
| ----------------------- | ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---- |
| connector               | string  | 连接器类型，取值仅支持"sequoiadb"     | 是   |
| hosts                   | string  | SequoiaDB 集群中所有或部分协调节点地址，格式为"sdbserver1:11810,sdbserver2:11810..." <br> 配置多个地址时，需确保地址所指向的协调节点存在于同一集群中 | 是   |
| collectionspace         | string  | 所映射的集合空间名称                  | 是   |
| collection              | string  | 所映射的集合名称                      | 是   |
| username                | string  | SequoiaDB 用户名，默认值为""          | 否   |
| passwordtype            | string  | 输入用户密码时对应的密码类型，默认值为"cleartext"，取值如下：<br>"cleartext"：参数 password 为明文密码 <br>"file"：参数 password 为密码文件路径      | 否   |
| token                   | string  | 使用 SequoiaDB 密码管理工具保存密码到密码文件时所指定的加密令牌    | 否   |
| password                | string  | 与参数 username 对应的 SequoiaDB 用户密码                          | 否   |
| bulksize                | int32   | 将数据从 Flink 插入 SequoiaDB 时，单次允许插入的记录条数，默认值为 500      | 否   |
| maxbulkfilltime         | int64   | 当插入记录条数不满足参数 bulksize 的取值时，操作等待的超时时间，默认值为 300，单位为秒<br> 等待超时后数据将被写入，建议根据业务能容忍的最大延迟进行配置  | 否   |
| splitmode               | string  | 分片生成模式，默认值为"auto"，取值如下：<br>"auto"：自动选择模式 <br>"sharding"：以分区为单位进行并发读取 <br> "datablock"：以集合为单位进行并发读取 <br> 该参数取值为"auto"时，如果查询使用了索引则自动选择"sharding"模式，未使用索引则选择"datablock"模式        | 否   |
| preferredinstance       | string  | 分区优先选择的节点实例，默认值为"M"<br>取值说明可参考 [setSessionAttr()][setSessionAttr] 的参数 PreferredInstance  | 否   |
| preferredinstancemode   | string  | 当多个实例符合参数 preferredinstance 条件时的选择模式，默认值为"random"<br>取值说明可参考 [setSessionAttr()][setSessionAttr] 的参数 PreferredInstanceMode | 否   |
| preferredinstancestrict | boolean | 参数 preferredinstance 指定的实例 ID 都不符合条件时是否报错，默认值为 false，表示不报错    | 否   |
| ignorenullfield         | boolean | 向表中插入数据时忽略值为 null 的字段，默认值为 false，表示不忽略值为 null 的字段             | 否   |
| pagesize                | int32   | insert into select 创建集合空间时指定数据页大小，默认值为 65536 <br> 如果集合空间已存在则忽略该参数               | 否   |
| domain                  | string  | insert into select 创建集合空间时指定所属域 <br> 如果集合空间已存在则忽略该参数              | 否   |
| group                   | string  | insert into select 创建集合时指定创建在某个复制组<br>所指定的复制组必须存在于集合空间所属的域中                                                         | 否   |
| autopartition           | boolean | insert into select 创建集合时指定是否为分区集合，默认值为 true，表示创建分区集合，并按如下规则进行自动分区：<br> 1）优先根据分区键进行自动分区 <br> 2）如果未指定分区键，将根据主键进行自动分区 <br>  3）如果未指定分区键和主键，将创建普通集合      <br> 该参数不能与参数 group 同时使用         | 否   |
| shardingkey             | json    | insert into select 创建集合时指定分区键 | 否   |
| shardingtype            | string  | insert into select 创建集合时指定分区类型，默认值为"hash"，表示散列分区 <br> 该参数目前仅支持取值为"hash" |
| replsize                | int32   | insert into select 创建集合时指定副本写入数  | 否   |
| compressiontype         | string  | insert into select 创建集合时指定压缩类型，默认值为"lzw"，取值如下：<br> "none"：关闭压缩 <br> "lzw"：lzw 算法压缩 <br> "snappy"：snappy 算法压缩       | 否   |
|  parallelism            | int32   | Sink 并发度，默认值为 1，取值应小于当前 Flink 集群的总 Slot 数量 <br> 建议取值为 SequoiaDB 集群中协调节点数量的倍数                           | 否   |  
| overwrite           | boolean | Sink 是否开启覆写，默认值为 true，表示开启 <br> 该参数仅在 append-only 模式下有效，建议取值如下： <br> 1）在批量写入的场景下，建议取值为 false，以提高写入效率  <br>  2）在实时写入的场景下，建议取值为 true，以保证数据一致性 <br> 取值为 true 时，需保证 Flink 映射表存在主键，且所映射的 SequoiaDB 集合存在对应的唯一索引                                                              | 否   |
| writemode               | string  | 数据写入模式，默认值为"append-only"，取值如下：<br> "append-only"：仅支持 INSERT 操作 <br> "upsert"：支持 INSERT、UPDATE、DELETE 操作，但不支持变更主键 <br> "retract"：在"upsert"模式的基础上，支持主键更新操作 <br> 根据使用场景建议取值如下：<br> 1）在追加写入的场景下，建议取值为"append-only"，以提高写入效率 <br> 2）增量数据更新、数据加工及其混合场景下，建议取值为"upsert"或"retract"，此时需要保证所操作的记录存在主键 | 否 |
| sink.retract.partitioned-source  | boolean  | 增量数据更新场景下，上游数据源是否为分区数据源（例如 Kafka 多分区 Topic），默认值为 false，表示不为多分区数据源<br>该参数仅在 writemode 取值为"retract"时有效 | 否 |
| sink.retract.event-ts-field-name | string  | 事件时间对应的字段名<br>当参数 sink.retract.partitioned-source 取值为 true 并生效时，用户需从业务字段中选取 TIMESTAMP(6) 或更高精度的时间戳作为事件时间，以保证增量数据更新结果正确 | 否 |
| sink.retract.state-ttl | int32  | 状态存活时间，默认值为 1，单位为 min<br>当参数 sink.retract.partitioned-source 取值为 true 并生效时，用户需设置状态存活时间，同时开启 Flink Checkpoint 机制，避免因状态无法保存导致增量数据结果不准确 | 否 |

##保证精确一次性##

FlinkSQL 连接器通过 Checkpoint 机制保证数据写出的精确一次性。当用户将表配置 overwrite 设置为 true 时，需根据实际场景配置 Flink 集群中与 StateBackend 有关的参数，以保证数据处理的精确一次。

下述以使用 RocksDB 增量存储 Checkpoint 为例，演示部分参数的配置方式，用户需根据实际情况调整配置。

1. 配置 Checkpoint 间隔，以启用 Checkpoint 机制

    ```lang-ini
    execution.checkpointing.inteval: 5 min
    ```

2. 配置 StateBackend 相关参数

    ```lang-ini
    state.backend: rocksdb
    state.backend.incremental: true
    state.checkpoints.dir: hdfs:///flink-checkpoint        # location to store checkpoints
    ···
    ```

3. 确保已正确配置 RocksDB 中的相关参数






[^_^]:
    本文使用的所有引用及链接
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md