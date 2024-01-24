
使用 hint 可以显式地控制执行方式或者执行计划。

##语法##
___/*+hint1 hint2 ...*/___

当希望控制某个语句时，只需要在这个语句结尾处增加 hint 即可。属于同一个语句的 hint 使用空格分隔。

hint有多种不同类型：

| hint类型    | 描述                       |
| ----------- | -------------------------- |
| use_hash    | 指定关联方式为哈希关联    |
| use_index   | 指定集合的扫描方式        |
| use_flag    | 设置标志位                |
| use_option  | 设置查询参数，主要用于[监控视图][snapshot] |

##类型描述##

###use_hash###

* 语法

  *use_hash()*

* 参数描述

  无

* 支持语句

  select语句

###use_index###

* 语法

  *use_index( \<index_name\> )*

  *use_index( \<cl_name, index_name\> )*

* 参数描述

  | 参数名      | 参数类型 | 描述                                    | 是否必填 |
  | ----------- | -------- | --------------------------------------- | -------- |
  | index_name  | string   | 索引名                                 | 是       |
  | cl_name     | string   | 集合名                                 | 是       |

* 支持语句

  select语句

###use_flag###

* 语法

  *use_flag( \<flag_name\> )*

* 参数描述

  | 参数名    | 参数类型         | 描述                            | 是否必填 |
  | --------- | ---------------- | ------------------------------- | -------- |
  | flag_name | string \| number | 标志位，不同语句的 flag 取值不同 | 是       |

* 支持语句

  | 语句   | flag取值                                 | 描述                       |
  | ------ | ---------------------------------------- | -------------------------- |
  | update | SQL_UPDATE_KEEP_SHARDINGKEY (0x00008000) | 更新条件中保留分区键字段  |


##返回值##

无

##示例##

   * 数据库中集合 sample.employee1, sample.employee2, sample.employee3 的情况如下：

   ```lang-json
   // sample.employee1包含索引 "idx_employee1_a"，该索引以 "a" 字段升序排序，其记录如下：
   { "a": 0 }
   { "a": 1 }
   { "a": 2 }
   { "a": 3 }
   { "a": 4 }

   // sample.employee2包含索引 "idx_employee2_b"，该索引以 "b" 字段升序排序，其记录如下：
   { "a": 1, "b": 1 }
   { "a": 2, "b": 1 }
   { "a": 3, "b": 2 }
   { "a": 4, "b": 2 }
   { "a": 5, "b": 2 }

   // sample.employee1包含索引 "idx_employee3_c"，该索引以 "c" 字段升序排序，其记录如下：
   // sample.employee3
   { "c": 0 }
   { "c": 1 }
   { "c": 2 }
   { "c": 3 }
   { "c": 4 }
   ```

###use_hash()###

   * 指定关联方式为哈希关联 

   ```lang-javascript
   > db.exec("select t1.a, t2.b from sample.employee1 as t1 inner join sample.employee2 as t2 on t1.a = t2.b /*+use_hash()*/")
   { "a": 1, "b": 1 }
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   Return 5 row(s).
   ```

###use_index()###

   * 指定集合的扫描方式

   * 使用集合 sample.employee1 中索引 "idx_employee1_a" 进行扫描 

   ```lang-javascript
   > db.exec("select * from sample.employee1 where a = 1 /*+use_index(idx_employee1_a)*/")
   { "_id": { "$oid": "582ae8ea790ce72860000023" }, "a": 1 }
   Return 1 row(s).
   ```

   * 在关联中指定集合 sample.employee1 使用索引 "idx_employee1_a" 进行扫描 

   ```lang-javascript
   > db.exec("select t1.a, t2.b from sample.employee1 as t1 inner join sample.employee2 as t2 on t1.a = t2.b /*+use_index(t1, idx_employee1_a)*/")
   { "a": 1, "b": 1 }
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   Return 5 row(s).
   ```

   * 在一个 select 语句中使用多个 hint 

   ```lang-javascript
   > db.exec("select t1.a, t2.b from sample.employee1 as t1 inner join sample.employee2 as t2 on t1.a = t2.b /*+use_index(t1, idx_employee1_a) use_index(t2, idx_employee2_b) use_hash()*/")
   { "a": 1, "b": 1 }
   { "a": 1, "b": 1 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   { "a": 2, "b": 2 }
   Return 5 row(s).
   ```

   * 指定集合不使用索引 

   ```lang-javascript
   > db.exec("select * from sample.employee1 where a = 1 /*+use_index(NULL)*/")
   { "_id": { "$oid": "582ae8ea790ce72860000023" }, "a": 1 }
   Return 1 row(s).
   ```

###use_flag()###

   * 指定更新记录时保留分区键 

   * 存在切分表 sample.employee，落在两个复制组上，分区键为 { b: 1 }

   ```lang-javascript
   > db.execUpdate( "update sample.employee set b = 1 where age < 25 /*+use_flag(SQL_UPDATE_KEEP_SHARDINGKEY)*/" )
   (nofile):0 uncaught exception: -178
   Sharding key cannot be updated
   > db.execUpdate( "update sample.employee set b = 1 where age < 25 /*+use_flag(0x00008000)*/" )
   (nofile):0 uncaught exception: -178
   Sharding key cannot be updated
   ```

###use_option###

设置查询参数，主要用于[监控视图][snapshot]。

* 语法

  *use_option( \<option_name\>, \<option_value\> )*

* 支持参数

  | 选项   | 对应监控视图 | 描述                       |
  | ------ | ------------ | -------------------------- |
  | Mode | [配置快照视图](manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md) | 指定返回配置的模式。在 run 模式下，显示当前运行时配置信息，在 local 模式下，显示配置文件中配置信息。如 ```use_option(Mode,local)```。默认为 run。 |
  | Expand | [配置快照视图](manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md) | 是否扩展显示用户未配置的配置项。如 ```use_option(Expand,false)```。默认为 true。 |
  | ShowError | ALL | 指定是否返回错误信息。在 show 模式下，显示错误信息，在 only 模式下，只显示错误信息，不显示其他快照信息，在 ignore 模式下，不显示错误信息。如 ```use_option(ShowError,only)```。默认为 show。 |
  | ShowErrorMode | ALL | 指定返回错误信息的格式。在 aggr 模式下，错误信息聚合为一条记录显示，在 flat 模式下，一个错误节点对应一条记录显示。如 ```use_option(ShowErrorMode,flat)```。默认为 aggr。 |

* 示例

查看数据组 db1 中数据节点 20000 上的用户指定的配置信息

```lang-javascript
> db.exec('select * from $SNAPSHOT_CONFIGS where GroupName = "db1" and ServiceName = "20000" /*+use_option(Mode, local) use_option(Expand, false)*/') 
{
  "NodeName": "hostname:20000",
  "dbpath": "/opt/test/20000/",
  "ServiceName": "20000",
  "diaglevel": 3,
  "role": "data",
  "catalogaddr": "hostname:30003,hostname:30013,hostname:30023",
  "sparsefile": "TRUE",
  "plancachelevel": 3,
  "businessname": "yyy",
  "clustername": "xxx"
}
```

查看数据组 db1 各个节点的配置信息，并且把错误信息显示为单独一条记录

```lang-javascript
> db.exec('select * from $SNAPSHOT_CONFIGS where GroupName = "db1" /*+use_option(Mode, local) use_option(Expand, false) use_option(ShowError,show) use_option(ShowErrorMode,flat)*/')
{
  "NodeName": "hostname:20000",
  "GroupName": "db1",
  "Flag": -79,
  "ErrInfo": {}
}
{
  "NodeName": "hostname:21000",
  "GroupName": "db1",
  "Flag": -79,
  "ErrInfo": {}
}
{
  "NodeName": "hostname:22000",
  "dbpath": "/opt/database/22000/",
  "ServiceName": "22000",
  "diaglevel": 5,
  "role": "data",
  "catalogaddr": "hostname:30003,hostname:30013,hostname:30023",
  "sparsefile": "TRUE",
  "plancachelevel": 3
}
```


[^_^]:
    本文使用的所有引用及链接
[snapshot]:manual/Manual/SQL_Grammar/Monitoring/Readme.md