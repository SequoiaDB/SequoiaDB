[^_^]:
     PostgreSQL 实例-库与表操作

本文档主要介绍一些简单的库与表操作示例，更多操作可参考 [PostgreSQL 官网][PostgreSQL]。

##关联 SequoiaDB 集合空间与集合##

创建表 test，并关联 SequoiaDB 的集合 sample.employee

```lang-sql
sample=# create foreign table test (name text, id numeric) server sdb_server options ( collectionspace 'sample', collection 'employee', decimal 'on' );
```

关联 SequoiaDB 集合空间与集合时，可配置的参数如下：

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| collectionspace | string | SequoiaDB 中已存在的集合空间 | 是 |
| collection | string | SequoiaDB 中已存在的集合 | 是 |
| decimal | string | 是否对接 SequoiaDB 的 decimal 字段，默认为'off' | 否 |
| pushdownsort | string | 是否下压排序条件到 SequoiaDB，默认为'on'，启用下压 | 否 |
| pushdownlimit | string | 是否下压 limit 和 offset 条件到 SequoiaDB，默认为'on'，启用下压 <br> 参数 pushdownlimit 为'on'时，参数 pushdownsort 必须为'on' ，否则可能会造成结果非预期的问题 | 否 |

>**Note:**
>
> 在 PostgreSQL 中建立相应的映射表关联 SequoiaDB 集合时，需要确保映射表的字段名与集合的字段名大小写一致，且映射表的字段类型与集合的字段类型一致；否则，将查询不到相关数据。

##更新统计信息##

更新表 test 的统计信息

```lang-sql
sample=# analyze test;
```

##查看所有表##

查看数据库 sample 中的所有表

```lang-sql
sample=# \d
```

##查看表的描述信息##

查看表 test 的描述信息

```lang-sql
sample=# \d test
```

##删除表的映射关系##

删除表 test 的映射关系

```lang-sql
sample=# drop foreign table test;
```



[^_^]:
    本文使用到的所有连接及引用
[PostgreSQL]:https://www.postgresql.org/docs/9.3/index.html