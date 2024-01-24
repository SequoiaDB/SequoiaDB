[^_^]:
     PostgreSQL 实例-CRUD 操作

本文档主要介绍一些简单的 CRUD 操作示例，更多操作可参考 [PostgreSQL 官网][PostgreSQL]。

##查询##

查询表 test 中的所有记录

```lang-sql
sample=# select * from test;
```

> **Note:**
>
> 如果 PostgreSQL 所连接的 SequoiaDB 协调节点重启，则需要重新进入 PostgreSQL Shell，否则查询报错。

##插入##

在表 test 中插入如下记录

```lang-sql
sample=# insert into test values('one',3);
```

##更新##

匹配表 test 中 name 为'one'的记录，并将该记录中字段 id 的值更新为 9

```lang-sql
sample=# update test set id=9 where name='one';
```

##删除##

删除表 test 中字段 id 为 2 的记录

```lang-sql
sample=# delete from test where id=2;
```



[^_^]:
    本文使用到的所有连接及引用
[PostgreSQL]:https://www.postgresql.org/docs/9.3/index.html
