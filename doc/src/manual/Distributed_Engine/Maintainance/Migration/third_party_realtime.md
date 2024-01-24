[^_^]:
    实时第三方数据复制
    作者：谢建宏
    时间：20190816


随着机器学习和人工智能的发展，越来越多的企业倾向于实时的获取数据的价值，而不满足于通过夜间运行批量任务作业的方式来处理信息。
本文档将介绍如何将 DB2、Oracle、MySQL 的数据实时复制至 SequoiaDB 巨杉数据库中。

DB2和Oracle数据实时复制
----

对于 DB2 和 Oracle 的数据实时复制存在很多种方案，通常的做法如下：
- 使用 Oracle 官方数据迁移工具，如 OGG(Oracle GlodenGate)、CDC（Change Data Capture）
- 自研数据导入导出程序实现
- 使用第三方数据迁移工具

MySQL数据实时复制
----

SequoiaDB 作为分布式数据库，由数据库存储引擎与数据库实例两大模块构成。
其中，数据库存储引擎模块是数据存储的核心，负责提供整个数据库的读写服务、数据的高可用与容灾、ACID 与分布式事务等全部核心数据服务能力。

用户可以通过添加 SequoiaSQL-MySQL 实例，基于 binlog Replication 方式实时同步 MySQL 的数据至 SequoiaDB 中。
在 MySQL 与 SequoiaDB 之间建立主从复制需要以下步骤：

1. 安装部署 Sequoiasql-MySQL，详细步骤参见 [SequoiaSQL-MySQL 安装部署][mysql_deploy]
2. 存量数据从 MySQL 迁移至 SequoiaSQL-MySQL
3. MySQL数据库开启 binlog 日志，配置为主库
4. SequoiaSQL-MySQL 开启 binlog 日志，配置为从库
5. 配置 binlog Replication 主从关系

### 存量数据从 MySQL 迁移至 Sequoiadb-MySQL

MySQL 的 binlog Replication 机制只能实时同步增量数据，不能同步存量数据，因此存量数据需要使用 mydumper 工具导出，再使用 myloader 工具导入到 SequoiaSQL-MySQL 中。可参考 [mydumper&myloader 的使用][mydumper&myloader]。

SequoiaSQL-MySQL 采用的存储引擎是 SequoiaDB 分布式数据库引擎，而非 InnoDB 引擎，对于 mydumper 导出的建表语句需要进行相应的修改，因此需要分别导出数据表结构以及数据。
假设 MySQL 数据库中存在存量数据 info 库，将该库的数据迁移至 SequoiaSQL-MySQL 操作步骤如下：

1. 导出 info 库的所有表结构

   ```lang-bash
   $ mydumper -h sdbserver1 -P 3306 -u sdbadmin -p sdbadmin -d -t 4 -s 1000000 -e -B info -o /home/sdbadmin/info/schema
   ```

2. 导出 info 库的所有数据表的数据

   ```lang-bash
   $ mydumper -h sdbserver1 -P 3306 -u sdbadmin -p sdbadmin -m -t 4 -s 1000000 -e -B info -o /home/sdbadmin/info/data
   ```

3. 修改表结构的存储引擎为 SequoiaDB，字符编码修改为 utf8mb4

4. 导入 info 库的表结构至 SequoiaSQL-MySQL

   ```lang-bash
   $ myloader -h sdbserver2 -P 3306 -u sdbadmin -p sdbadmin -t 4 -d /home/sdbadmin/info/schema
   ```

5. 导入 info 库的数据表数据至 SequoiaSQL-MySQL 的 info 库中

   ```lang-bash
   $ myloader -h sdbserver2 -P 3306 -u sdbadmin -p sdbadmin -t 4 -d /home/sdbadmin/info/data
   ```

### MySQL 开启 binlog ###

1. 修改 MySQL 配置文件 `/etc/mysql/mysql.conf.d/mysqld.cnf`

   ```lang-ini
   [mysqld]
   port=3306
   log-bin=master-bin
   server-id=1
   ```

2. 使用 root 用户权限重启 MySQL 服务

   ```lang-bash
   # service mysql restart
   ```

3. 查看 binlog 日志状态

   ```lang-sql
   mysql> show variables like '%log_bin%';
   ```

   输出结果如下：

   ```lang-text
   +---------------------------------+------------------------------------------------------+
   | Variable_name                   | Value                                                |
   +---------------------------------+------------------------------------------------------+
   | log_bin                         | ON                                                   |
   | log_bin_basename                | /opt/sequoiasql/mysql/database/3306/master-bin       |
   | log_bin_index                   | /opt/sequoiasql/mysql/database/3306/master-bin.index |
   | log_bin_trust_function_creators | OFF                                                  |
   | log_bin_use_v1_row_events       | OFF                                                  |
   | sql_log_bin                     | ON                                                   |
   +---------------------------------+------------------------------------------------------+
   6 rows in set (0.00 sec)
   ```

### SequoiaSQL-MySQL 开启 binlog

1. 修改 SequoiaSQL-MySQL 配置文件 `/opt/sequoiasql/mysql/database/3306/auto.cnf`

   ```lang-ini
   [mysqld]
   server-id=3
   relay_log=relay-log
   relay_log_index=relay-log.index
   ```

2. 使用 root 用户权限重启 SequoiaSQL-MySQL 服务

   ```lang-bash
   # service sequoiasql-mysql restart
   ```

3. 查看 replay_log 日志状态

   ```lang-sql
   mysql> show variables like '%relay_log%';
   ```

   输出结果如下：

   ```lang-text
   +---------------------------+-----------------------------------------------------+
   | Variable_name             | Value                                               |
   +---------------------------+-----------------------------------------------------+
   | max_relay_log_size        | 0                                                   |
   | relay_log                 | relay-log                                           |
   | relay_log_basename        | /opt/sequoiasql/mysql/database/3306/relay-log       |
   | relay_log_index           | /opt/sequoiasql/mysql/database/3306/relay-log.index |
   | relay_log_info_file       | relay-log.info                                      |
   | relay_log_info_repository | FILE                                                |
   | relay_log_purge           | ON                                                  |
   | relay_log_recovery        | OFF                                                 |
   | relay_log_space_limit     | 0                                                   |
   | sync_relay_log            | 10000                                               |
   | sync_relay_log_info       | 10000                                               |
   +---------------------------+-----------------------------------------------------+
   11 rows in set (0.06 sec)
   ```

### 配置 Binlog Recplication 主从关系 ###

1. 在 MySQL 主库上查看主库 binlog 日志文件位置

   ```lang-sql
   mysql> show master status\G
   ```

   输出结果如下：

   ```lang-text
   *************************** 1. row ***************************
                File: master-bin.000001
            Position: 154
        Binlog_Do_DB:
    Binlog_Ignore_DB:
   Executed_Gtid_Set:
   1 row in set (0.00 sec)
   ```

2. 在主库上授权复制用户

   ```lang-sql
   mysql> grant replication slave,replication client on *.* to 'repl'@'%' identified by 'sequoiadb';
   mysql> flush privileges;
   ```

3. 在 SequoiaSQL-MySQL 从库上配置主从关系，使用有复制权限的用户账号连接主库，启动复制线程

   ```lang-sql
   reset slave;
   change master to
   master_host='sdbserver1',
   master_user='repl',
   master_password='sequoiadb',
   master_port=3306,
   master_log_file='master-bin.000001',
   master_log_pos=154;
   start slave;
   ```

4. 查看 Slave 状态

   ```lang-sql
   mysql> show slave status\G
   ```

   输出结果如下：

   ```lang-text
   *************************** 1. row ***************************
                  Slave_IO_State: Waiting for master to send event
                     Master_Host: sdbserver1
                     Master_User: repl
                     Master_Port: 3306
                   Connect_Retry: 60
                 Master_Log_File: master-bin.000001
             Read_Master_Log_Pos: 154
                  Relay_Log_File: relay-log.000002
                   Relay_Log_Pos: 321
           Relay_Master_Log_File: master-bin.000001
                Slave_IO_Running: Yes
               Slave_SQL_Running: Yes
                 Replicate_Do_DB:
             Replicate_Ignore_DB:
              Replicate_Do_Table:
          Replicate_Ignore_Table:
         Replicate_Wild_Do_Table:
     Replicate_Wild_Ignore_Table:
                      Last_Errno: 0
                      Last_Error:
                    Skip_Counter: 0
             Exec_Master_Log_Pos: 154
                 Relay_Log_Space: 522
                 Until_Condition: None
                  Until_Log_File:
                   Until_Log_Pos: 0
              Master_SSL_Allowed: No
              Master_SSL_CA_File:
              Master_SSL_CA_Path:
                 Master_SSL_Cert:
               Master_SSL_Cipher:
                  Master_SSL_Key:
           Seconds_Behind_Master: 0
   Master_SSL_Verify_Server_Cert: No
                   Last_IO_Errno: 0
                   Last_IO_Error:
                  Last_SQL_Errno: 0
                  Last_SQL_Error:
     Replicate_Ignore_Server_Ids:
                Master_Server_Id: 1
                     Master_UUID: dec14b1d-b772-11e9-af76-0050562a7848
                Master_Info_File: /opt/sequoiasql/mysql/database/3306/master.info
                       SQL_Delay: 0
             SQL_Remaining_Delay: NULL
         Slave_SQL_Running_State: Slave has read all relay log; waiting for more updates
              Master_Retry_Count: 86400
                     Master_Bind:
         Last_IO_Error_Timestamp:
        Last_SQL_Error_Timestamp:
                  Master_SSL_Crl:
              Master_SSL_Crlpath:
              Retrieved_Gtid_Set:
               Executed_Gtid_Set:
                   Auto_Position: 0
            Replicate_Rewrite_DB:
                    Channel_Name:
              Master_TLS_Version:
   1 row in set (0.03 sec)
   ```

### mydumper&myloader 安装 ###

mydumper&myloader 是用于对MySQL数据库进行多线程备份和恢复的开源 (GNU GPLv3）工具。安装部署步骤如下：

1. 到 [mydumper 官网][mydumper_download]下载 mydumper安装包

2. 切换到 root 权限用户，执行安装命令

- Centos6 / Red Hat6：

   ```lang-bash
   # sudo yum install mydumper-0.9.5-2.el6.x86_64.rpm
   ```
- Centos7 / Red Hat7：

   ```lang-bash
   # sudo yum install mydumper-0.9.5-2.el7.x86_64.rpm
   ```

- Ubuntu / Debian：

   ```lang-bash
   # sudo dkpg -i mydumper_0.9.5-2.xenial_amd64.deb
   ```

小结
----
SequoiaDB 巨杉数据库支持通过 Oracle 官方迁移工具、第三方迁移工具等方式从 DB2、Oracle 中实时同步数据至 SequoiaDB 以及支持基于 MySQL 的 binlog Replication 机制实时复制 MySQL 的数据至 SequoiaDB 中。

[^_^]:
    本文使用的所有引用和链接
[mysql_deploy]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Installation/install_deploy.md
[mydumper&myloader]:http://blog.sequoiadb.com/cn/detail-id-102
[mydumper_download]: https://github.com/maxbube/mydumper