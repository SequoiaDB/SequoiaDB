[^_^]:
    MariaDB 实例-注意事项

本文档将介绍 MariaDB 实例组件的注意事项。

- MariaDB 实例不支持创建外键。

- 时间戳类型字段取值范围是 1970-01-01 00:00:00~2038-01-19 03:14:07。

- 索引键不能超过 3072 字节，通过 MariaDB 实例创建的索引，不可直接在 SequoiaDB 上对索引执行删除或修改操作。

- 复合唯一索引仅支持所有字段 null 值重复，不允许部分字段 null 值重复，例如：允许出现(null,null)和(null,null)重复值，但不允许出现(1,null)和(1,null)重复值。

- 用户建表时默认创建 SequoiaDB 分区表。若需要在分区表建立新的唯一索引，则唯一索引必须包含其分区键，如需更改创建分区表的配置可参考[配置说明][config]。

- MariaDB 实例不支持在 BINARY、VARBINARY、TINYBLOB、BLOB、MEDIUMBLOB、LONGBLOB、JSON 和 GEOMETRY 类型的字段上创建索引。

- MariaDB 实例不支持对接独立模式的 SequoiaDB。

- 一个 MariaDB 实例节点仅可与一个 SequoiaDB 集群对接，不支持同时对接多个 SequoiaDB 集群。

- VARCHAR 和 TEXT 在 SequoiaDB 上进行查询比较时不会忽略尾部空格，而 MariaDB 会忽略尾部空格，因此对于尾部含有空格的字符串，查询结果可能会不准确。

- DDL操作不支持事务功能。

- 对于 LIMIT ROWS EXAMINED 语法，SequoiaDB 引擎不保证其运行结果与原生 MariaDB 一致。

- MariaDB 实例不支持 YEAR(2) 类型的字段。

- MariaDB 实例不支持在虚拟列上创建索引。

- MariaDB 实例不支持指定虚拟列为分区键。

- MariaDB 实例仅支持 utf8mb4_bin 和 utf8_bin 校对集。

- MariaDB 实例支持自增字段，MariaDB 表自增字段对应 SequoiaDB 的集合[自增字段][sequence]，只保证趋势递增，不保证连续递增，使用时需注意以下事项:
    * auto_increment_offset：该配置项主要解决多活主网下自增字段冲突问题，而 SequoiaDB 作为分布式数据库，自身能保证自增字段全局递增而不冲突，故该配置项不生效。
    * auto_increment_increment：自增字段数值由 SequoiaDB 生成，当发生了创建表以外的行为（例如插入数据），该配置项不起作用。如需要修改步长，可以在 SDB Shell 中通过 [setAttributes()][setAttributes] 修改对应的自增序列步长属性。
    * 自增字段类型为无符号长整型时，取值范围是 0~9223372036854775807。
    * 自增字段数值增长到自身类型最大值时，会报“序列值超出范围”错误且插入数据失败。
    
- MariaDB 实例支持序列，对应 SequoiaDB 的[序列][sequence]，只保证趋势递增，不保证连续递增，使用时需注意以下事项:
    * auto_increment_increment、auto_increment_offset：这两个配置主要解决多实例下序列的冲突问题，而 SequoiaDB 的序列，自身能保证序列全局递增而不冲突，故这两个配置不生效。
    * 序列的 increment 属性不能为 0。
    * 序列的 cache_size 属性不能为 0。


[^_^]:
     本文使用的所有引用和链接
[config]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Operation/database_and_table_operation.md#自定义表配置
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md
