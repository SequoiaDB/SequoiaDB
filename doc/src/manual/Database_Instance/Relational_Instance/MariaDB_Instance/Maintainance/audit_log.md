审计日志记录了用户对数据库执行的所有操作。通过审计日志，用户可以对数据库进行故障分析、行为分析和安全审计等操作，能有效帮助用户获取数据库的执行情况。

##安装审计插件##

MariaDB 通过审计插件输出审计日志，因此在审计前需完成审计插件的安装。审计插件安装后仅对当前实例生效，如果用户希望在多个实例中进行审计，需在每个实例中分别安装审计插件。

下述以实例 myinst（实例端口号为 6101）、实例组件安装目录 `/opt/sequoiasql/mariadb` 为例，介绍具体安装步骤。

1. 切换至数据库管理用户（安装时创建，默认为 sdbadmin），并进入 MariaDB 实例组件的安装目录

    ```lang-bash
    # su - sdbadmin
    $ cd /opt/sequoiasql/mariadb
    ```

2. 创建审计日志的存放目录

    以创建目录 `auditlog/6101` 为例，执行如下语句：

    ```lang-bash
    $ mkdir -p auditlog/6101
    ```

3. 编辑实例配置文件，并根据实际情况调整配置参数的取值

    ```lang-bash
    $ vim /opt/sequoiasql/mariadb/database/6101/auto.cnf
    ```
    在文件末尾添加以下内容：

    ```lang-ini
    # 加载审计插件
    plugin-load=server_audit=server_audit.so
    # 开启审计功能
    server_audit_logging=ON
    # 配置审计日志所记录的操作类型为 conncet，query
    server_audit_events=connect,query
    # 配置审计日志路径及文件名
    server_audit_file_path=/opt/sequoiasql/mariadb/auditlog/6101/server_audit.log
    # 限制审计日志文件的大小为 10MB，超过该大小时将进行轮转
    server_audit_file_rotate_size=10485760
    # 限制审计日志的保留个数为 20 个，超过后会丢弃最早的日志文件
    server_audit_file_rotations=20
    # 限制每行查询日志的大小为 100KB，若表比较复杂，对应的操作语句比较长，建议增大该值
    server_audit_query_log_limit=102400
    ```

    > **Note:**
    >
    > 示例中仅列举部分配置参数，完整的配置参数列表可参考 [MariaDB 官网][configure]。

4. 重启实例

    ```lang-bash
    $ bin/sdb_maria_ctl restart myinst
    ```

5. 连接实例 myinst

    ```lang-bash
    $ bin/mariadb --socket=/opt/sequoiasql/mariadb/database/6101/mysqld.sock -u sdbadmin
    ```

6. 查看审计配置

    ```lang-sql
    MariaDB [(none)]> show variables like 'server_audit%';
    ```
    输出结果如下：

    ```lang-text
    +-------------------------------+------------------------------------------------------------+
    | Variable_name                 | Value                                                      |
    +-------------------------------+------------------------------------------------------------+
    | server_audit_events           | CONNECT,QUERY                                              |
    | server_audit_excl_users       |                                                            |
    | server_audit_file_path        | /opt/sequoiasql/mariadb/auditlog/6101/server_audit.log     |
    | server_audit_file_rotate_now  | OFF                                                        |
    | server_audit_file_rotate_size | 10485760                                                   |
    | server_audit_file_rotations   | 20                                                         |
    | server_audit_incl_users       |                                                            |
    | server_audit_logging          | ON                                                         |
    | server_audit_mode             | 0                                                          |
    | server_audit_output_type      | file                                                       |
    | server_audit_query_log_limit  | 102400                                                     |
    | server_audit_syslog_facility  | LOG_USER                                                   |
    | server_audit_syslog_ident     | mysql-server_auditing                                      |
    | server_audit_syslog_info      |                                                            |
    | server_audit_syslog_priority  | LOG_INFO                                                   |
    +-------------------------------+------------------------------------------------------------+
    ```

##监控审计插件##

安装审计插件后，用户可以对审计插件的运行状态进行监控，具体操作和说明可参考 [MariaDB 官网][monitor]。

##卸载审计插件##

下述以实例 myinst 为例，介绍具体卸载步骤。

1. 编辑实例 myinst 对应的实例配置文件

    ```lang-bash
    $ vim /opt/sequoiasql/mariadb/database/6101/auto.cnf
    ```
    在文件末尾将以下内容删除：

    ```lang-ini
    # 加载审计插件
    plugin-load=server_audit=server_audit.so
    # 开启审计功能
    server_audit_logging=ON
    # 配置审计日志所记录的操作类型为 conncet，query
    server_audit_events=connect,query
    # 配置审计日志路径及文件名
    server_audit_file_path=/opt/sequoiasql/mariadb/auditlog/6101/server_audit.log
    # 限制审计日志文件的大小为 10MB，超过该大小时将进行轮转
    server_audit_file_rotate_size=10485760
    # 限制审计日志的保留个数为 20 个，超过后会丢弃最早的日志文件
    server_audit_file_rotations=20
    # 限制每行查询日志的大小为 100KB，若表比较复杂，对应的操作语句比较长，建议增大该值
    server_audit_query_log_limit=102400
    ```

2. 重启实例

    ```lang-bash
    $ bin/sdb_maria_ctl restart myinst
    ```

3. 连接实例 myinst

    ```lang-bash
    $ bin/mariadb --socket=/opt/sequoiasql/mariadb/database/6101/mysqld.sock -u sdbadmin
    ```

4. 查看审计插件是否已卸载

    ```lang-bash
    MariaDB [(none)]> show variables like 'server_audit%';
    ```
    输出如下信息表示已卸载：

    ```
    Empty set (0.001 sec)
    ```
	








[^_^]:
     本文使用的所有引用及链接
[configure]:https://mariadb.com/kb/en/mariadb-audit-plugin-options-and-system-variables/
[monitor]:https://mariadb.com/kb/en/mariadb-audit-plugin-status-variables/
