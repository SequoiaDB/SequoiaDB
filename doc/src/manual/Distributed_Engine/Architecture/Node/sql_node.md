
SQL 实例是系统提供 SQL 访问能力的逻辑节点，可以直接配置 MySQL、PostgreSQL 和 SparkSQL 实例，实现不同 SQL 访问方式。

SQL 实例支持水平伸缩，实例互相独立。当接收到外部请求时，SQL 实例会进行 SQL 解析并生成内部的执行计划，执行计划将下发至协调节点，汇总协调节点的应答后进行外部响应。一次外部请求只能在一个 SQL 实例内完成，因此，可以根据外部应用的压力来规划 SQL 实例的规模。

> **Note:**
>
> SQL 实例需要进行一定的配置，才可以对接至指定的数据库存储引擎。

## 操作 ##

下述以 PostgreSQL 为例，介绍 SQL 实例的相关操作。

- 创建 SQL 节点

   指定实例名为 myinst，该实例名映射相应的数据目录和日志路径（用户可根据需要指定不同的实例名）

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D pg_data/
   ```

   SequoiaSQL PostgreSQL 默认启动端口为 5432，若端口号被占用，用户可以使用 `-p` 参数指定实例端口号

   ```lang-bash
   $ bin/sdb_pg_ctl addinst myinst -D pg_data/ -p 5433
   ```

- 启动 SQL 节点

   ```lang-bash
   $ bin/sdb_pg_ctl start myinst
   Starting instance myinst ...
   ok (PID: 20502)
   ```

- 查看 SQL 节点

    ```lang-bash
   $ bin/sdb_pg_ctl status
   INSTANCE   PID      SVCNAME   PGDATA                               PGLOG                                   
   myinst     20502     5432      /opt/sequoiasql/postgresql/pg_data   /opt/sequoiasql/postgresql/pg_data/myinst.log     
   Total: 1; Run: 1
   ```

- 对接 SequoiaDB 引擎

  系统默认数据库名为 postgres，用户也可以创建指定的数据库，命令如下：

   ```lang-bash
   $bin/sdb_pg_ctl createdb sample myinst
   Creating database myinst ...
   ok
   ```

   连接至数据库，如果没有创建指定的数据库，则连接默认数据库即可

   ```lang-bash
   $bin/psql -p 5432 sample
   ```

   > **Note:**
   >
   > 相关的配置操作，可参考 [SQL 引擎连接配置][connection]。

- 停止 SQL 节点

   ```lang-bash
   $ bin/sdb_pg_ctl stop myinst
   Stoping instance myinst (PID: 20502) ...
   ok
   ```

- 删除 SQL 节点

   ```lang-bash
   $ bin/sdb_pg_ctl delinst myinst
   Deleting instance myinst ...
   ok
   ```

[^_^]:
      本文使用的所有引用和链接
[connection]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/connection.md
