[^_^]:
    目录名：添加SQL实例

1. 进入【Sequoiaperf 配置】页面后选择【SequoiaPerf 配置】选项卡，点击 **添加-SQL 实例** 按钮并填写表格

 ![添加SQL实例][add_sql_instance]


    - SQL 实例组：MySQL 实例所属的实例组，如果不指定，则不属于任何实例组，显示为 Null
    - 主机 IP：SequoiaPerf 实例可以访问的 MySQL 主机 IP，不能填写主机名或 127.0.0.1
    - 端口：MySQL 实例端口号
    - 用户名：对 Performance_schema 表具有 SELECT 权限的 MySQL 用户名
    - 密码：与指定用户名对应的 MySQL 用户密码
    - 慢查询阈值：设置用于过滤慢速查询的阈值，默认值为 300ms；低于该阈值的慢查询不会被计入历史查询信息中
    - 抓取慢查询：是否开启捕获慢速查询，默认值为 on
    - 缓存队列大小：在缓冲区中保留的 SQL 慢查询最大数量，默认值为 1000

2. 点击 **提交** 按钮完成 SQL 实例的添加

![添加成功][add_sql_instance_succeed]


##确认用户权限##

SQL 实例添加完成后，用户需确认所使用的用户是否对 Performance_schema 表具有 SELECT 权限。如果权限不足，SeqipoPerf 将无法获取监控数据。

检查用户 `MySQL_USER` 的权限

```lang-sql
mysql> SHOW GRANTS FOR 'MySQL_USER';
```

输出结果如下：

```lang-text
+----------------------------------------------------------------------+
| Grants for MySQL_USER@%                                              |
+----------------------------------------------------------------------+
| GRANT SELECT ON `performance_schema`.* TO 'MySQL_USER'@'%'           |
+----------------------------------------------------------------------+
1 rows in set (0.00 sec)
```

如果指定用户不具有相关权限，可以使用如下语句设置权限：

```lang-sql
mysql> create user 'MySQL_USER'@'%' IDENTIFIED BY 'MySQL_PASSWORD';
mysql> grant select on performance_schema.* to MySQL_USER@'%' IDENTIFIED by 'MySQL_PASSWORD';
```


[^_^]:
    本文使用的所有引用及链接
[deployment]:manual/SequoiaPerf/Deployment/sequoiaperf_deployment.md
[management]:manual/SequoiaPerf/Deployment/sequoiaperf_management.md

[add_sql_instance]:images/SequoiaPerf/Configuration/add_sql_instance.png
[add_sql_instance_succeed]:images/SequoiaPerf/Configuration/add_sql_instance_succeed.png
