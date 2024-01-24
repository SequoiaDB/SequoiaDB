本文档将介绍 SequoiaDB 巨杉数据库中 MariaDB 实例的相关配置。

##SequoiaDB 引擎配置修改方式##

配置参数有以下三种修改方式：

- 通过工具 sdb_maria_ctl 修改配置

    ```lang-bash
    $ bin/sdb_maria_ctl chconf myinst --sdb-auto-partition=OFF
    ```

- 通过实例数据目录下的配置文件 `auto.cnf`，在[mysqld]一栏添加/更改对应配置项
 
    ```lang-ini
    sequoiadb_auto_partition=OFF
    ```

    > **Note:** 
    > 
    > 修改配置文件后需要重新启动 MariaDB 服务

- 通过 MariaDB 命令行修改

    ```lang-sql
    MariaDB [company]> SET GLOBAL sequoiadb_auto_partition=OFF;
    ```

    > **Note:** 
    >
    > 通过命令行方式修改的配置为临时有效，当重启 MariaDB 服务后配置将失效，若需要配置永久生效则必须通过配置文件的方式修改。 

##SequoiaDB 引擎配置使用说明##

###配置 SequoiaDB 连接与鉴权###

**sequoiadb_conn_addr** 

该参数可以配置 MariaDB 实例所连接的 SequoiaDB 存储集群，可以配置一个或多个协调节点的地址。使用多个时，地址之间要以逗号隔开。如 `sdbserver1:11810,sdbserver2:11810`。在配置多个地址时，每次连接会从地址中随机随机选择。在 MariaDB 会话数很多时，压力会基本平均地分摊给每个协调节点。

+ 类型：string
+ 默认值："localhost:11810"
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_user**

该参数可以配置 SequoiaDB 集群鉴权的用户。SequoiaDB 鉴权支持明文密码和密码文件两种方式，建议采用密码文件的方式建立连接。

+ 类型：string
+ 默认值：""
+ 作用范围：Global
+ 是否支持在线修改生效：是
  

**sequoiadb_password**

该参数可以配置 SequoiaDB 集群鉴权的明文密码。

+ 类型：string
+ 默认值：""
+ 作用范围：Global
+ 是否支持在线修改生效：是
  

**sequoiadb_token** 和 **sequoiadb_cipherfile**

这两个参数可以配置 SequoiaDB 集群鉴权的加密口令和密码文件路径。在配置前，需通过 sdbpassword 工具生成密码文件，具体可参考[数据库密码工具][sdbpasswd]章节。

+ 类型：string
+ 默认值：sequoiadb_token：""，sequoiadb_cipherfile："~/sequoiadb/passwd"
+ 作用范围：Global
+ 是否支持在线修改生效：是

> **Note:**
>
>* 以上配置在命令行修改后，均在建立新连接时才生效，不影响旧连接。
>* 两种密码都配置的情况下，优先使用明文密码。

###配置自动分区功能###

**sequoiadb_auto_partition**

该参数可以配置 MariaDB 是否使用自动分区功能。自动分区可以普遍提升 SequoiaDB 的性能。启动时，在 MariaDB 上创建表将同步在 SequoiaDB 上创建对应的分区表（散列分区，包含所有复制组）。自动分区时，分区键按顺序优先使用主键字段和唯一索引字段。如果两者都没有，则不做分区。

如果开启自动分区后，部分表不希望被分区，可以在[自定义表配置][config]中指定 auto_partition 为 false。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global
+ 是否支持在线修改生效：是

> **Note:**
>
> 自动分区时，主键或唯一索引只在建表时对应分区键，建表后添加、删除主键或唯一索引都不会更改分区键。

###配置默认副本数###

**sequoiadb_replica_size** 

该参数可以配置表默认的写操作需同步的副本数，取值范围为[-1,7]。副本数多时，数据一致性强度高，但性能会有所下降；副本数少时，则反之。具体可参考 SequoiaDB 创建集合的 [ReplSize 参数][createCL]。

+ 类型：int32
+ 默认值：1
+ 作用范围：Global
+ 是否支持在线修改生效：是

###配置批量插入###

**sequoiadb_use_bulk_insert**

该参数可以配置是否开启批量插入功能。批量插入可以提升 SequoiaDB 存储引擎的插入性能。在关闭该功能时，MariaDB 的批量插入在 SequoiaDB 中是逐条的插入；而开启时，SequoiaDB 存储引擎会把 MariaDB 的一个批次分解成若干个 sequoiadb_bulk_insert_size 大小的批次进行插入。例如，MariaDB 批量插入 1024 条记录，在 sequoiadb_bulk_insert_size 为 100 时，SequoiaDB 存储引擎会进行 10 次记录数为 100 的批量插入，和 1 次记录数为 24 的批量插入。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_bulk_insert_size**

该参数可以配置 SequoiaDB 每次进行批量插入的记录数，取值范围为[1,100000]。在进行插入性能的调优时，可以根据实际适当调整这个值。

+ 类型：int32
+ 默认值：2000
+ 作用范围：Global
+ 是否支持在线修改生效：是

###配置性能优化参数###

**sequoiadb_selector_pushdown_threshold**

该参数可以配置查询字段下压的触发阈值，取值范围为[0,100]。查询字段不下压时，SequoiaDB 集群总是返回完整记录给 MariaDB，由 MariaDB 过滤有用字段；而在查询字段下压时，SequoiaDB 集群只返回 MariaDB 所需字段。在查询字段个数/表总字段个数的百分比小于等于该阈值时查询字段下压，否则不下压。下压查询字段虽然节省了网络传输，但同时也会增加 SequoiaDB 工作量，可以根据实际适当调整。

+ 类型：uint32
+ 默认值：30
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_optimizer_options**

该参数可以配置是否开启优化操作。可填选项如下：<br>
"direct_count"：将 count 语句直接下压到 SeuoiaDB 执行。优化前，SELECT COUNT(*) 会请求 SequoiaDB 返回表中的所有记录，由 MySQL 进行计数；优化后，SELECT COUNT(*) 会对接到 SequoiaDB 的 [SdbCollection.count()][count] 方法，由 SequoiaDB 进行计数。<br>
"direct_update"：将 update 语句直接下压到 SeuoiaDB 执行。优化前，MySQL 会先查询匹配记录，然后逐条记录地下发更新请求。优化后，在符合条件的场景下，只需下发一次更新请求，从而减少网络 IO。<br>
"direct_delete"：将 delete 语句直接下压到 SeuoiaDB 执行。原理与 direct_update 相似，可以减少网络IO。<br>
"direct_sort"：将 order by 和 group by 直接下压到 SeuoiaDB 执行。优化前，排序操作在单个 MySQL 实例上完成。优化后，排序操作由 SequoiaDB 完成。得益于 SequoiaDB 多节点并发排序的能力，性能可以得到提升。<br>
"direct_limit"：将 limit 和 offset 直接下压到 SequoiaDB 执行。优化前，SequoiaDB 需返回所有匹配记录，limit 和 offset 操作在 MySQL 实例进行。优化后，在符合条件的场景下，SequoiaDB 只需返回 limit 指定的记录数。该选项与 direct_sort 结合使用，可以极大地提升分页查询效率。


+ 类型：set
+ 默认值："direct_count,direct_delete,direct_update,direct_sort,direct_limit"
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**optimizer_limit_pushdown_threshold**

该参数可以配置 limit 下压的阈值。当查询语句是带有 order by 和 limit 子句的块驱动关联查询时，允许 limit 下压可以减少内表扫描的次数，以提升性能。该阈值需根据查询语句中 limit 的取值（limit_num），以及 order by 列所在的表经评估后确定的检查行数（rows）进行配置。当 rows/limit_num 大于阈值时，limit 子句将被下压。用户可根据实际场景调整该参数，找到适配场景的最优取值，保证集群的高性能运行。

+ 类型：uint32 
+ 默认值：100 
+ 取值范围为[1, 2^32 -1]
+ 作用范围：Global,Session 
+ 是否支持在线修改生效：是 

###配置事务功能###

**sequoiadb_use_transaction**

该参数可以配置事务功能。在业务无需事务功能时，可设置为 OFF，从而节省不必要的开销。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global，Session
+ 是否支持在线修改生效：是

**sequoiadb_rollback_on_timeout** 

该参数可以配置记录锁超时是否中断并回滚整个事务。设置为开启后，遇到记录锁超时错误后会中断并且回滚整个事务，否则只会回滚最后一条 SQL 语句。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_lock_wait_timeout**

该参数可以配置事务锁等待超时时间。

+ 类型：int32
+ 默认值：60
+ 取值范围：[0,3600]
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_use_rollback_segments**

该参数可以配置事务是否使用回滚段。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global, Session
+ 是否支持在线修改生效：是

###配置统计信息分析###

**sequoiadb_stats_mode**

该参数可以配置分析（ANALYZE TABLE）模式。<br>
取值如下：<br>
1：表示进行抽样分析，生成统计信息 <br>
2：表示进行全量数据分析，生成统计信息 <br>
3：表示生成默认的统计信息 <br>
4：表示加载统计信息到 SequoiaDB 缓存中 <br>
5：表示清除 SequoiaDB 缓存的统计信息

+ 类型：int32
+ 默认值：1
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_sample_num**

该参数可以指定抽样的记录个数，取值范围为[100,10000]，指定 0 表示缺省。该参数不能与 sequoiadb_stats_sample_percent 同时指定。

+ 类型：int32
+ 默认值：200
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_sample_percent**

该参数可以指定抽样的比例，取值范围为[0.0,100.0]，指定 0.0 表示缺省。表记录数和比例的乘积为抽样的记录数。个数会自动调整在 100~10000 之间（小于 100 调整为 100，大于 10000 调整为 10000）。该参数不能与 sequoiadb_stats_sample_num 同时指定。

+ 类型：double
+ 默认值：0.0
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_cache**

该参数可以配置是否加载 SequoiaDB 统计信息到 MariaDB 缓存。统计信息缓存可以帮助生成更高效的访问计划，但会有少量的加载开销。关闭时，则使用默认规则生成访问计划，不使用统计信息。

+ 类型：boolean
+ 默认值：ON
+ 作用范围：Global
+ 是否支持在线修改生效：是

**sequoiadb_stats_cache_level**

该参数可以配置 SequoiaDB 统计信息加载到 MariaDB 缓存的级别。取值为 1 时，表示加载基础的索引统计信息，加载快且内存占用少，可以用于简单均匀数据模型的估算。取值为 2 时，表示加载索引频繁数值集合（Most Common Values，MCV）统计信息，可以用于基于样本的估算，使多种数据模型下的估算都更加准确。

+ 类型：uint32
+ 默认值：2
+ 作用范围：Global, Session
+ 是否支持在线修改生效：是

**sequoiadb_stats_flush_time_threshold**

该参数可以配置 MariaDB 实例清理统计信息缓存的时间阈值，单位为小时，仅在开启实例组功能后生效。对表执行 CRUD 操作时，实例将比对该表上一次生成统计信息的时间。如果时间间隔超过该阈值，实例将清理统计信息缓存后再执行当前操作。实例自动清理表的统计信息缓存后，将通知同实例组的其他实例执行相同操作。该参数配置为 0 时，表示不自动清理统计信息缓存。

+ 类型：int32
+ 默认值：0
+ 取值范围：[0,720]
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

###配置 SequoiaDB 节点优先级###

**sequoiadb_preferred_instance**

该参数可以配置 MariaDB 会话进行读操作时，优先选择的 SequoiaDB 节点，取值规则可参考 [PreferredInstance][setSessionAttr] 参数说明。

+ 类型：string
+ 默认值："M"
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_preferred_instance_mode**

该参数可以配置多个节点符合 sequoiadb_preferred_instance 条件时，节点的选择模式，取值可参考 [PreferredInstanceMode][setSessionAttr] 参数说明。

+ 类型：string
+ 默认值："random"
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_preferred_strict**

该参数可以配置节点选取是否为严格模式。当为严格模式时，节点只能从 sequoiadb_preferred_instance 指定的规则中选取。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sequoiadb_preferred_period**

该参数可以配置优先节点的有效周期，单位为秒。如果上一次选择的节点在有效周期内，读请求仍使用该节点进行查询；有效周期之后，将根据 sequoiadb_preferred_instance 重新选择。

+ 类型：int32
+ 默认值：60
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

> **Note:**
>
> 事务模式下，所有操作均在主节点进行。因此上述配置需在无事务模式下修改，否则无效。

###配置元数据映射功能###

**sequoiadb_enable_mapping**

该参数可以配置 MariaDB 实例是否开启[元数据映射功能][metadata_mapping_management]。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global
+ 是否支持在线修改生效：否

**sequoiadb_mapping_unit_count**

该参数可以配置 MariaDB 表所映射的集合空间数量。

+ 类型：int32
+ 默认值：10
+ 取值范围：[10,50]
+ 块大小：10
+ 作用范围：Global
+ 是否支持在线修改生效：否

**sequoiadb_mapping_unit_size**

该参数可以配置单个集合空间支持创建的集合数量。

+ 类型：int32
+ 默认值：1024
+ 取值范围：[1024,2048]
+ 块大小：512
+ 作用范围：Global
+ 是否支持在线修改生效：否

###配置 TCP 探活###

MariaDB 实例组件基于 TCP 协议中的 KeepAlive 机制，以发送探测包的方式检测服务端与客户端间的连接状态。当客户端与服务端长时间未交互时，服务端将向客户端发送探测包以确认连接是否存活。

**tcp_keepalive_time**

该参数可以配置服务端等待与客户端交互的时间，单位为秒。当超出指定时间后两端仍未交互，服务端将发送第一个探测包。该参数配置为 0 时，表示取值与 Linux 系统中对应配置的值一致。

+ 类型：uint32
+ 默认值：0
+ 作用范围：global
+ 是否支持在线修改：是

**tcp_keepalive_interval**

该参数可以配置服务端发送探测包的时间间隔，单位为秒。该参数配置为 0 时，表示取值与 Linux 系统中对应配置的值一致。

+ 类型：uint32
+ 默认值：0
+ 作用范围：global
+ 是否支持在线修改：是

**tcp_keepalive_probes**

该参数可以配置服务端发送探测包的次数。如果在指定次数内客户端仍未响应，对应连接将被清理。该参数配置为 0 时，表示取值与 Linux 系统中对应配置的值一致。

+ 类型：uint32
+ 默认值：0
+ 作用范围：global
+ 是否支持在线修改：是

###其它配置###

**sequoiadb_alter_table_overhead_threshold**

该参数可以配置表开销阈值。当表记录数超过这个阈值，需要全表更新的更改操作将被禁止。这个限制是为了防止对大表误进行更改操作，因为大表的更新会花费较多的时间。该阈值对添加 DEFAULT NULL 的列、数据类型扩容等无需更新的轻量操作不生效。如确认要对大表结构进行更改，在线上调阈值后，重新执行更改操作即可。

+ 类型：int64
+ 默认值：10000000
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是
       

**sequoiadb_execute_only_in_mysql**

该参数可以配置 DQL/DML/DDL 语句只在 MariaDB 执行，不会下压到 SequoiaDB 执行。即 DDL 只会变更 MariaDB 的表元数据信息，而不会变更 SequoiaDB 相应表元数据；DQL/DML 所有查询和变更都为空操作，不会实际查询和修改 SequoiaDB 相应表的数据。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Session
+ 是否支持在线修改生效：是
  
**sequoiadb_execution_mode**

该参数可以配置 SQL 语句执行模式。该参数设置为 0 时，DDL/DML/DQL 操作均会下发到 SequoiaDB，且 DDL 操作将同步到实例组内的其他实例；该参数设置为 1 时，DDL/DML/DQL 操作均不会下发到 SequoiaDB，且 DDL 操作不同步到实例组内的其他实例；该参数设置为 2 时，DDL/DML/DQL 操作均不会下发到 SequoiaDB，但 DDL 操作将同步到实例组内的其他实例。

+ 类型：int32
+ 默认值：0
+ 取值范围：[0, 2]
+ 作用范围：Session
+ 是否支持在线修改生效：是

**sequoiadb_debug_log**

该参数可以配置 MariaDB 日志是否会打印 SequoiaDB 存储引擎有关 debug 信息。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是
  

**sequoiadb_error_level**

该参数可以配置错误级别，可选的配置项有 error 和 warning，用于控制连接器的某些错误返回的方式（报错或警告）。当 SQL 语句执行出错时，若该参数配置为 error ，则连接器直接返回错误信息给客户端；若参数配置为 warning ，则连接器返回警告信息给客户端。用户可根据 warning 查询详细的错误信息。该参数仅适用于 update ignore 更新分区键失败时的错误信息。

+ 类型：enum
+ 默认值：error
+ 作用范围：Global
+ 是否支持在线修改生效：是


**sequoiadb_support_mode**

该参数可以配置 MariaDB 实例的行为模式，目前支持的模式如下：

"strict_on_table"：表级别的强校验模式。该模式仅支持创建 utf8_bin 或 utf8mb4_bin 校对集的表，且不支持创建外键。如果不指定该模式，则支持创建所有校对集的表，同时忽略创建外键的相关定义。不指定该模式时，MariaDB 不保证相关语句执行的正确性。

+ 类型：set
+ 默认值：strict_on_table
+ 作用范围： Global,Session
+ 是否支持在线修改生效：是

**sql_select_result_limit**

该参数可以配置 select 语句执行后，期望返回的最大记录数。当查询到的记录数大于或等于该参数指定的值时，系统将依据参数 sql_select_result_limit_exceed_handling 的配置，做出相应的行为。

+ 类型：uint64
+ 默认值：18446744073709551615
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**sql_select_result_limit_exceed_handling**

该参数可以配置 select 语句执行后返回记录的行为。当查询到的记录数大于或等于参数 sql_select_result_limit 指定的值时，系统将依据配置项返回相应的信息，可选配置项为 NONE、WARNING 和 ERROR。配置为 NONE 时，表示直接返回查询到的记录；配置为 WARNING 时，表示返回查询到的记录并携带警告信息；配置为 ERROR 时，表示仅返回报错信息。

+ 类型：enum
+ 默认值：NONE
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

**information_schema_tables_stats_cache_first**

该参数可以配置读取 information_schema.tables 中所涉及的统计信息字段时，是否优先从缓存中获取。

+ 类型：boolean
+ 默认值：OFF
+ 作用范围：Global,Session
+ 是否支持在线修改生效：是

##MariaDB 常用系统配置##

| 参数名                 | 类型   | 动态生效 | 动态范围   | 默认值  | 说明 |
| ---------------------- | ----   | -------- | ---------- | ------- | ---- |
| max_connections        | int32  | Yes | Global          | 1024    | 客户端最大连接数 |
| max_prepared_stmt_count| int32  | Yes | Global          | 128000  | 最大预编译语句数 |
| sql_mode               | set    | Yes | Global,Session | STRICT_TRANS_TABLES,<br>ERROR_FOR_DIVISION_BY_ZERO,<br>NO_AUTO_CREATE_USER,<br>NO_ENGINE_SUBSTITUTION | SQL 模式，取值意义可参考[MariaDB SQL 模式][sql_mode] |
| character_set_server   | string | Yes | Global,Session | utf8mb4 | 默认字符集 |
| collation_server       | string | Yes | Global,Session | utf8mb4_bin | 默认校对集 |
| default_storage_engine | string | Yes | Global,Session | SequoiaDB | 默认存储引擎 |
| lower_case_table_names | int32  | No  | Global          | 1       | 表名大小写策略，取值如下：<br>0：表名以原格式存储，比较时区分大小写<br>1：表名以小写格式存储，比较时不区分大小写<br>2：表名以原格式存储，以小写进行比较|
| join_cache_level       | int32  | Yes | Global,Session  | 8       | 连接缓存级别，取值为 [0,8]；如果级别设置为 0，表示不使用任何级别的连接算法；如果级别设置为 4，表示使用 4 或 4 以下级别（即可以使用 [0,4] 级别）的连接算法，其它取值同理<br>各级别对应的连接算法如下：<br>1：表示 Flat BNL <br>2：表示 Incremental BNL <br>3：表示 Flat BNLH <br>4：表示 Incremental BNLH <br>5：表示 Flat BKA <br>6：表示 Incremental BKA <br>7：表示 Flat BKAH <br>8：表示 Incremental BKAH<br> 上述算法可参考 [MariaDB 连接算法][Block_based_join_algorithms] |
| optimizer_switch       | flagset| Yes | Global,Session | mrr=on,mrr_cost_based=off,<br>join_cache_incremental=on,<br>join_cache_hashed=on,<br>join_cache_bka=on,<br>optimize_join_buffer_size=on,<br>index_merge_intersection=off | 优化器开关，取值意义可参考 [MariaDB 优化器开关][optimizer_switch]   |
| transaction_isolation  | enum   | Yes | Global,Session  | REPEATABLE-READ | 事务隔离级别，取值可参考 [MariaDB 事务隔离级别配置][trans_isolation]<br>SequoiaDB v3.2.x/v3.4.x 版本不支持 RR 隔离级别，因此将 MariaDB 实例的隔离级别配置为 REPEATABLE-READ 并连接至 SequoiaDB 时，隔离级别会自动降级为 READ-COMMITTED |

> **Note:** 
>
> * 在系统最大文件句柄数不足时，max_connections 可能被自动调整。如果发现修改该配置没有生效，可检查系统 limit 设置和 MariaDB 日志。
> * SequoiaDB 不支持大小写敏感的校对集。
> * 在 Linux 平台下，需谨慎变更配置 lower_case_table_names。如果随意变更该配置，可能导致匹配不到原表。


[^_^]:
    本文使用的所有引用和链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[config]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Operation/database_and_table_operation.md#自定义表配置
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[sdbpasswd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md#引擎配置
[count]:manual/Manual/Sequoiadb_Command/SdbCollection/count.md
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[sql_mode]:https://mariadb.com/kb/en/sql-mode/
[optimizer_switch]:https://mariadb.com/kb/en/library/optimizer_switch/
[Block_based_join_algorithms]:https://mariadb.com/kb/en/library/block-based-join-algorithms/
[trans_isolation]:https://mariadb.com/kb/en/server-system-variables/#tx_isolation
[metadata_mapping_management]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/metadata_mapping_management.md
