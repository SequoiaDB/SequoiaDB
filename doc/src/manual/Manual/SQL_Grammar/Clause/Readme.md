SequoiaDB 巨杉数据库中 SQL 语句可以嵌套子句进行数据库操作，支持的子句如下：

| 语句 | 描述 | 示例 |
|------|------|------|
| [where][where] | 过滤记录 | db.exec("select * from sample.employee where age > 10") |
| [group by][group_by] | 对结果集进行分组 | db.exec("select dept_no, count(emp_no) as 员工总数 from sample.employee group by dept_no") |
| [order by][order_by] | 对结果集排序 | db.exec( "select dept_no,count(emp_no) as 员工总数 from sample.employee group by dept_no order by dept_no desc" ) |
| [split by][split_by] | 拆分记录 | db.exec( "select a,b,c from sample.employee split by c" ) |
| [limit][limit] | 限制返回的记录个数 | db.exec("select * from sample.employee limit 2") |
| [offset][offset] | 设置跳过的记录数量 | db.exec("select * from sample.employee offset 3") |
| [as][as] | 为集合名、字段名或结果集指定别名 | db.exec("select age as 年龄 from sample.employee where age>10") |
| [inner join][inner_join] | 内连接 | db.exec( "select T1.a, T2.b from sample.employee1 as T1 inner join sample.employee2 as T2 on T1.a < 10" ) |
| [left outer join][left_outer_join] | 左连接 | db.exec("select t1.LastName, t1.FirstName, t2.OrderNo from sample.persons as t1 left outer  join sample.orders as t2 on t1.Id_P=t2.Id_P") |
| [right outer join][right_outer_join] | 右链接 | db.exec("select t1.LastName, t1.FirstName, t2.OrderNo from sample.persons as t1 right outer join sample.orders as t2 on t1.Id_P=t2.Id_P") |
| [hint][hint] | 显式地控制执行方式或者执行计划 | db.exec("select * from sample.employee1 where a = 1 /*+use_index(idx_employee1_a)*/") |

[^_^]:
    本文使用到的所有链接及引用
[where]:manual/Manual/SQL_Grammar/Clause/where.md
[group_by]:manual/Manual/SQL_Grammar/Clause/group_by.md
[order_by]:manual/Manual/SQL_Grammar/Clause/order_by.md
[split_by]:manual/Manual/SQL_Grammar/Clause/split_by.md
[limit]:manual/Manual/SQL_Grammar/Clause/limit.md
[offset]:manual/Manual/SQL_Grammar/Clause/offset.md
[as]:manual/Manual/SQL_Grammar/Clause/as.md
[inner_join]:manual/Manual/SQL_Grammar/Clause/inner_join.md
[left_outer_join]:manual/Manual/SQL_Grammar/Clause/left_outer_join.md
[right_outer_join]:manual/Manual/SQL_Grammar/Clause/right_outer_join.md
[hint]:manual/Manual/SQL_Grammar/Clause/hint.md