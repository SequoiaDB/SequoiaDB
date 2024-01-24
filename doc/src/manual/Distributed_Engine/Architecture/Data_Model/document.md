[^_^]:
    文档
    作者：谭钊波
    时间：20190701
    评审意见
    王涛：
    许建辉：
    市场部：20190821


SequoiaDB 巨杉数据库存储的记录（Record）也称为文档（Document）。文档在分布式存储引擎中，以 [BSON][data_mode_bson] 的方式存储，而 BSON 是 [JSON][data_mode_web_json] 数据模型的二进制编码。由于文档是一种基于 JSON 数据模型且支持嵌套结构与数据的灵活键值对，所以既可以存储关系型数据，也可以存储半结构化和非结构化数据。

文档概念
----

文档的字面形式如同 JSON 数据类型。而 JSON 是一种轻量级的数据交换格式，非常易于用户阅读和编写，同时也易于机器生成和解析。

文档结构
----

一般来说，一条文档由一个或多个字段构成，每个字段分为键与值两个部分，如下为包含两个字段的文档：

```lang-json
{ "姓名" : "张三", "性别" : "男" }
```

其中"姓名"和"性别"为字段的键，而"张三"和"男"为字段的值。

文档支持嵌套结构和数组，如下为包含嵌套结构和数组的文档：

```lang-json
{ "姓名" : "张三", "性别" : "男", "地址" : { "省份" : "yy", "城市" : "xx", "街道" : "水蓝街" }, "电话" : [ "138xxxxxxxx", "180xxxxxxxx" ] }
```

其中名为"地址"的字段的值为一个嵌套 JSON 对象, 而名为"电话"的字段的值为一个 JSON 数组。

> **Note:**
>
> 关于 JSON 的更多内容，可参考 [JSON 官网][data_mode_web_json]的介绍

字段的键与值
----

字段的键又称为字段名，其类型为字符串。

字段的值又称为字段值，可以为多种类型，如：整数、长整数、浮点数、字符串和对象等。

> **Note:**
>
> 关于字段值的类型可参考[数据类型][data_mode_data_type]章节的介绍。

文档的限制
----

文档存在如下限制：

- 每条文档的 BSON 编码最大长度为 16MB。
- 每条文档必须包括 _id 字段。如果用户没有提供该字段，系统会自动生成一个 [OID][data_mode_oid] 类型的 _id 字段。
- _id 字段名需要在集合内唯一（若集合为切分集合，则 _id 字段名需要在每个切分子集合内唯一）。
- 文档中各个字段无排列顺序，在进行数据操作时（例如进行数据更新），字段之间的顺序可能会被调换。

字段名存在如下限制：

- 用户插入的文档不允许包含重复的字段名。
- 用户自定义的字段名不允许以"$"字符起始。
- 用户自定义的字段名不允许包含"."字符。

> **Note:**
>
> - 由于 C/CPP BSON 能够构建重复字段名的 BSON, 而 Java/C# 等其它 BSON 无法构建重复字段名的 BSON，为了保证数据的正确性，不建议用户使用 C/CPP BSON 构建重复字段名的 BSON。
> - SequoiaDB 内部程序创建的一些文档（一般为元数据）可能含有重复字段名，但 SequoiaDB 不会向用户的文档添加重复字段名。

文档与SQL实例记录的比较
----

与 MySQL、PostgreSQL 等传统关系型数据库表定义需要指定记录的 schema 信息不同，SequoiaDB 的集合元数据并不会存储文档（记录）的 schema。在 SequoiaDB 中，文档的 schema 信息存储在文档自身当中。

文档是一种灵活的数据结构，其存储的数据包含了字段名和字段值。每个字段在 BSON 中的存储结构可以简化为如下格式：

![bson存储结构图][bson_storage_struct]

因此文档自身已经包含了足够的 schema 信息。


**示例**

通过在 MySQL 和 SequoiaDB 分别进行数据查询，比较两种记录组织方式的差异。

1. 在 MySQL Shell 创建数据库 sample 和表 employee，该数据库和表分别自动映射到 SequoiaDB 的集合空间 sample 和集合 employee 上：

   ```lang-sql
   mysql> create database sample;
   mysql> use sample;
   mysql> create table employee(name varchar(50), sex int, age int, department varchar(100));
   mysql> insert into employee values("Tom", 0, "25", "R&D");
   mysql> insert into employee values("Mary", 1, "23", "Test");
   mysql> select * from employee;
   ```
   查询结果如下：
   
   ```lang-text
   +-------+-------+-------+------------+
   | name  | sex   | age   | department |   
   +-------+-------+-------+------------|
   | "Tom" | 0     | 25    | "R&D"      |   
   | "Mary"| 1     | 23    | "Test"     | 
   +-------+-------+-------+------------|
   ```

2. 在 SequoiaDB 上查询插入的数据

   ```lang-bash
   sdb> db.sample.employee.find()
   ```
   
   查询结果如下：
   
   ```lang-json
   { "_id": { "$oid": "5d1da52b38892b0af758ee5f" }, "name": "Tom", "sex": 0, "age": 25, "department": "R&D" }
   { "_id": { "$oid": "5d1da53138892b0af758ee60" }, "name": "Mary", "sex": 1, "age": 23, "department": "Test" }
   ```


[^_^]:
     本文使用的所有链接及引用
[bson_storage_struct]:images/Distributed_Engine/Architecture/Data_Model/bson_storage_struct.png
[data_mode_data_type]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md
[data_mode_oid]:manual/Manual/Sequoiadb_Command/SpecialObjects/OID.md
[data_mode_bson]:http://www.bsonspec.org/
[data_mode_web_json]:http://json.org/json-zh.html
