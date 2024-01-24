[^_^]:
    MySQL 实例-注意事项

本文档将介绍使用 MySQL 实例组件的注意事项。

- MySQL 实例不支持创建外键。

- 时间戳类型字段取值范围为：1902-01-01 00:00:00.000000~2037-12-31 23:59:59.999999。

- 索引键不超过 3072 字节，通过 MySQL 实例创建的索引，不可直接在 SequoiaDB 上对索引执行删除或修改操作。

- 复合唯一索引仅支持所有字段 null 值重复，不允许部分字段 null 值重复，例如：允许出现(null,null)和(null,null)重复值，但不允许出现(1,null)和(1,null)重复值。

- 用户建表时默认创建 SequoiaDB 分区表。如需要在分区表建立新的唯一索引，则唯一索引必须包含其分区键；如需更改创建分区表配置见[配置说明][config]。

- MySQL 实例不支持在 BINARY、VARBINARY、TINYBLOB、BLOB、MEDIUMBLOB、LONGBLOB、JSON 和 GEOMETRY 类型的字段上创建索引。

- MySQL 实例不支持对接独立模式的 SequoiaDB。

- 一个 MySQL 实例节点仅可与一个 SequoiaDB 集群对接，不支持同时对接多个 SequoiaDB 集群。

- VARCHAR 和 TEXT 在 SequoiaDB 上进行查询比较时不会忽略尾部空格，而 MySQL 会忽略尾部空格，因此对于尾部含有空格的字符串，查询结果可能会不准确。

- DDL 操作不支持事务。

- MySQL 实例不支持在虚拟列上创建索引。

- MySQL 实例不支持指定虚拟列为分区键。

- MySQL 实例仅支持 utf8mb4_bin 和 utf8_bin 校对集。

- MySQL 实例支持自增字段，MySQL 表自增字段对应 SequoiaDB 的集合[自增字段][auto_increment]，只保证趋势递增，不保证连续递增，使用时需注意以下事项:
    * auto_increment_offset：该配置项主要解决多活主网下自增字段冲突问题，而 SequoiaDB 作为分布式数据库，自身能保证自增字段全局递增而不冲突，故该配置项不生效。
    * auto_increment_increment：自增字段数值由 SequoiaDB 生成，当发生了创建表以外的行为（例如插入数据），该配置项不起作用。如需要修改步长，可以在 SDB Shell 中通过 [SdbCollection.setAttributes()][setAttributes] 修改对应的自增序列步长属性。
    * 自增字段类型为无符号长整型时，取值范围是 0~9223372036854775807。
    * 自增字段数值增长到自身类型最大值时，会报“序列值超出范围”错误且插入数据失败。

[^_^]:
    本文使用到的所有连接及引用
[config]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Operation/database_and_table_operation.md#自定义表配置
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md
[auto_increment]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md#自增字段