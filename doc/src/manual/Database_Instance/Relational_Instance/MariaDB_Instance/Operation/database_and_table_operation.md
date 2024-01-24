[^_^]:
    MariaDB 实例-库与表操作

本文档主要介绍一些简单的库与表操作示例，更多操作可参考 [MariaDB 官网][mariadb]。

##创建数据库##

创建数据库 company，并使用该数据库

```lang-sql
MariaDB [(none)]> CREATE DATABASE company;
MariaDB [company]> USE company;
```

##创建表##

- 创建表 employee，指定字段 id 为自增字段，并作为该表的主键

    ```lang-sql
    MariaDB [company]> CREATE TABLE employee(id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(128), age INT);
    ```

- 创建表 manager，并在字段 employee_id 上建立名为“id_idx”的索引

    ```lang-sql
    MariaDB [company]> CREATE TABLE manager(employee_id INT, department TEXT, INDEX id_idx(employee_id));
    ```

###支持的建表选项###

用户在 MariaDB 上创建表时可以通过建表选项指定表属性。

| 选项 | 默认值 | 描述 |
| ---- | ------ | ---- |
| AUTO_INCREMENT | 1 | 自增字段的起始值，SequoiaDB 的自增字段不是严格递增，而是趋势递增；如果配置严格递增的自增字段，需将参数 AcquireSize 设置为 1，具体说明可参考 SequoiaDB [自增字段][sequence]章节 |
| CHARACTER SET | utf8mb4 | 字符数据的字符集 |
| COLLATE | utf8mb4_bin | 字符数据的比较规则，不支持忽略大小写的字符比较规则，字符比较对大小写敏感 |
| COMMENT | "" | 表备注信息，用于指定更多 SequoiaDB 引擎的选项，可参考下述的自定义表配置 |
| ENGINE | SEQUOIADB | 表存储引擎，必须指定为 SEQUOIADB 才能使用本分布式存储引擎，一般无需显式指定 |

**示例**

- 通过 COMMENT 创建压缩类型为"snappy"的表

    ```lang-sql
    MariaDB [db]> CREATE TABLE t2 (id INT) COMMENT='sequoiadb:{ table_options: { CompressionType: "snappy" } }';
    ```

- 指定表自增字段起始值为 1000，并严格递增

    ```lang-sql
    MariaDB [db]> CREATE TABLE tb (id INT AUTO_INCREMENT PRIMARY KEY) COMMENT='Strict auto_increment field for example, sequoiadb:{ table_options: { "AutoIncrement": { "Field": "id", "StartValue": 1000, "AcquireSize": 1 } } }';
    ```

###自定义表配置###

用户在 MariaDB 上创建表时，可以在表选项 COMMENT 中指定关键词"sequoiadb" ，并在其后添加一个 json 对象用于传入自定义的表配置参数。格式如下：

```lang-ini
COMMENT [=] "[string,] sequoiadb:{ [table_options:{...}, partition_options:{...}, auto_partition:<true|false>, mapping:<string>] }"
```

**具体配置参数**

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | --- | ------ | ------ |
| string | string |用户自定义注释字符串 | 否 |
| table_options | json | 创建集合的相关参数，详细参数可参考 [SequoiaDB 创建集合选项][createCL]| 否 |
| auto_partition | boolean | 是否创建分区表，取值为 false 则显式创建非分区表| 否 |
| mapping | string | 指定表与集合的[元数据映射][metadata_mapping_management]关系，格式为 `mapping: "<集合空间>.<集合>"`| 否 |

**示例**

- 在 SequoiaDB 上创建根据时间进行范围切分的表

    ```lang-sql
    MariaDB [company]> CREATE TABLE business_log(ts TIMESTAMP, level INT, content TEXT, PRIMARY KEY(ts))
        -> ENGINE=sequoiadb
        -> COMMENT="Sharding table for example, sequoiadb:{ table_options: { ShardingKey: { ts: 1 }, ShardingType: 'range' } }";
    ```

- 在引擎配置项 sequoiadb_auto_partition 为 ON 时，指定 auto_partition 为 false 显式创建普通表

    ```lang-sql
    MariaDB [company]> CREATE TABLE employee2(id INT PRIMARY KEY, name VARCHAR(128) UNIQUE KEY)
        -> ENGINE=sequoiadb 
        -> COMMENT='sequoiadb:{ auto_partition: false }';
    ```

##创建索引##

在表 employee 中创建名为“name_idx”的索引

```lang-sql
MariaDB [company]> ALTER TABLE employee ADD INDEX name_idx(name(30));
```

##删除表##

删除表 employee 和 manager

```lang-sql
MariaDB [company]> DROP TABLE employee, manager;
```

##删除数据库##

删除数据库 company

```lang-sql
MariaDB [company]> DROP DATABASE company;
```



[^_^]:
     本文使用的所有引用和链接
[config]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Operation/config.md
[mariadb]:https://mariadb.com/kb/en/documentation/
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[sdbpasswd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md#引擎配置
[metadata_mapping_management]:Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/metadata_mapping_management.md