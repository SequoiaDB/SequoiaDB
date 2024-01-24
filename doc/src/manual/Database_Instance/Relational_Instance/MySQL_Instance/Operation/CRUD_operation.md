[^_^]:
    MySQL 实例-CRUD 操作

本文档主要介绍一些简单的 CRUD 操作示例，更多操作可参考 [MySQL 官网][mysql]。

##插入##

分别在表 employee 和 manager 中插入如下记录：

```lang-sql
mysql> INSERT INTO employee(name, age) VALUES("Jacky", 36);
mysql> INSERT INTO employee(name, age) VALUES("Alice", 18);
mysql> INSERT INTO manager VALUES(1, "Wireless Business");
```

##查询##

**对结果集排序**

查询表 employee 的数据，并按指定条件对结果集进行排序

```lang-sql
mysql> SELECT * FROM employee ORDER BY id ASC LIMIT 1; 
```

**多表查询**

对表 employee 和 company 进行多表查询，返回 employee.id=manager.employee_id 的记录

```lang-sql
mysql> SELECT * FROM employee, manager WHERE employee.id=manager.employee_id;
```

##更新##

匹配表 employee 中 id 为 1 的记录，并将该记录中字段 name 的值更新为"Bob"

```lang-sql
mysql> UPDATE employee SET name="Bob" WHERE id=1;
```

##删除##

删除表 employee 中字段 id 为 2 的记录

```lang-sql
mysql> DELETE FROM employee WHERE id=2;
```



[^_^]:
     本文使用的所有引用和链接
[mysql]:https://dev.mysql.com/doc/refman/5.7/en/