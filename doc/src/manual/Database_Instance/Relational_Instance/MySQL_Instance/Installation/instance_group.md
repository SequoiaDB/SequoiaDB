[^_^]:
    MySQL 实例-实例组

MySQL 集群架构中，每个 MySQL 实例均为主机模式，都可对外提供读写服务。因此 MySQL 实例组件提供了实例组功能，用于为应用提供统一的元数据视图，并保证集群的元数据一致性。同时，当一个 MySQL 实例退出实例组后，连接该实例的应用可以切换到实例组内的其他实例，获得对等的读写服务，以保证服务的高可用。

##逻辑架构##

MySQL 实例组是由若干 MySQL 实例组成的一个无状态的集群，集群内部的实例可以相互同步元数据操作，用户可以通过部署多个实例来保证 MySQL 服务的高可用。MySQL 实例组架构图如下：

![实例组][instance_group]

实例组内 SQL 实例通过 HA 组件保证组内元数据的一致性。HA 主要功能如下：

+ 日志持久化：将用户的元数据操作日志写入到 SequoiaDB。
+ 日志回放：从 SequoiaDB 获取元数据操作日志，并在本地回放，回放操作不下发到 SequoiaDB。
+ 元数据全量同步：实例组功能允许新增 SQL 实例加入实例组。新增的 SQL 实例第一次启动时，需要从实例组中选取一个可用的 SQL 实例进行元数据全量同步操作。全量同步完成后，需要启动新增实例。如果实例组内没有实例，则直接启动新增实例。

> **Note：**
>
> 用户在使用实例组功能时，需确保 SequoiaDB 事务功能已开启（即通过[配置快照][sdb_snap_configs]查看 transactionon 为 true）。

##使用##

以下命令均在数据库管理用户（安装时创建，默认为 sdbadmin）下执行。

1. 进入到 MySQL 的安装目录

   ```lang-bash
   $ cd /opt/sequoiasql/mysql
   ```

2. 初始化一个名为“mysql”实例组

   ```lang-bash
   $ bin/ha_inst_group_init mysql --key test
   ```

3. 创建两个 MySQL 实例并加入到实例组 mysql

   ```lang-bash
   $ bin/sdb_mysql_ctl addinst myinst_01 -D database/3306 -P 3306 -g mysql -k test
   $ bin/sdb_mysql_ctl addinst myinst_02 -D database/3307 -P 3307 -g mysql -k test
   ```

   > **Note：**
   >
   > sdb_mysql_ctl 的使用可参考[实例管理][sdb_mysql_ctl]。

4. 查看实例组 mysql 的配置信息

   ```lang-bash
   $ bin/ha_inst_group_list --name mysql
   ```

   输出结果如下：

   ```lang-text
   InstanceGroupName    InstanceID    HostName     SvcName  DBType    DataGroup
   mysql                129135        sdbserver    3306     mysql     -
   mysql                129136        sdbserver    3307     mysql     -
   ```

5. 使用 mysql 命令连接到 myinst_01 实例

   ```lang-bash
   $ bin/mysql --socket=/opt/mysql/database/3306/mysqld.sock -u root
   ```

6. 在实例 myinst_01 上进行建库、建表等元数据操作

   ```lang-sql
   mysql> create database mysql_ha_test;
   Query OK, 1 row affected (0.42 sec)
   
   mysql> use mysql_ha_test
   Database changed
   mysql> create table t1(id int);
   Query OK, 0 rows affected (0.35 sec)
   
   mysql> create table t2 as select * from t1;
   Query OK, 0 rows affected (0.76 sec)
   Records: 0  Duplicates: 0  Warnings: 0
   
   mysql> create table t3 like t1;
   Query OK, 0 rows affected (0.51 sec)
   
   mysql> show tables;
   +-------------------------+
   | Tables_in_mysql_ha_test |
   +-------------------------+
   | t1                      |
   | t2                      |
   | t3                      |
   +-------------------------+
   3 rows in set (0.01 sec)
   ```

7. 使用 mysql 命令连接到 myinst_02 实例

   ```lang-bash
   $ bin/mysql --socket=/opt/mysql/database/3307/mysqld.sock -u root
   ```

8. 验证实例 myinst_01 的操作是否同步到实例 myinst_02

   ```lang-sql
   mysql> show tables from mysql_ha_test;
   +-------------------------+
   | Tables_in_mysql_ha_test |
   +-------------------------+
   | t1                      |
   | t2                      |
   | t3                      |
   +-------------------------+
   3 rows in set (0.00 sec)
   ```

9. 新增实例 myinst_03 并加入实例组
 
   ```lang-bash
   $ bin/sdb_mysql_ctl addinst myinst_03 -D database/3308 -P 3308 -g mysql -k test
   ```
 
10. 使用 mysql 命令连接到实例 myinst_03

   ```lang-bash
   $ bin/mysql --socket=/opt/mysql/database/3308/mysqld.sock -u root
   ```
 
11. 查询元数据是否同步到实例 myinst_03

   ```lang-sql
   mysql> show tables from mysql_ha_test;
   +-------------------------+
   | Tables_in_mysql_ha_test |
   +-------------------------+
   | t1                      |
   | t2                      |
   | t3                      |
   +-------------------------+
   3 rows in set (0.00 sec)
   ```

##配置参数##

**server_ha_inst_group_name**

该参数用于配置当前实例所属的实例组。若没有配置该参数，默认不启用实例组功能。

+ 类型：string
+ 默认值：""
+ 作用范围：Global
+ 是否支持在线修改生效：否

**server_ha_inst_group_key**

该参数用于加密实例组用户密码。实例启动时，会从默认的配置表中获取实例组用户的配置信息，并在当前实例中创建实例组用户。实例组用户可用于执行全量元数据同步和日志回放操作。

+ 类型：string
+ 默认值：""
+ 作用范围：Global
+ 是否支持在线修改生效：否

**server_ha_wait_recover_timeout**

该参数用于设置等待元数据全量同步的超时时间。启用实例组功能的情况下，新增实例启动时需要与实例组中其他实例进行元数据的全量同步。为保证元数据的一致性，在该实例未完成同步时，外部元数据操作请求将进入等待状态。如果等待超时，则返回错误提示信息。

+ 类型：uint32
+ 单位：秒
+ 默认值：30
+ 作用范围：Global
+ 取值范围：[0,3600]
+ 是否支持在线修改生效：是

**server_ha_wait_replay_timeout**

该参数用于设置等待元数据日志回放完成的超时时间。启用实例组功能的情况下，实例启动时会在后台创建一个日志回放线程，日志回放线程定期从 SequoiaDB 获取元数据操作日志并在本地回放。由于日志的回放操作是定期执行的，在一个实例上更改元数据并不能实时地同步到其他实例。因此，在一个实例上修改元数据后，立即在另外一个实例上更改相同对象的元数据时，需要等待最新的元数据同步完成后才能进行。如果等待超时，则返回错误提示信息。

+ 类型：uint32
+ 单位：秒
+ 默认值：30
+ 作用范围：Global
+ 取值范围：[0,3600]
+ 是否支持在线修改生效：是

**server_ha_wait_sync_timeout**

该参数用于设置实例组内，等待其他实例同步当前实例元数据操作的超时时间。启用实例组功能的情况下，如果该参数的值不为 0，则实例成功执行元数据操作后，需要等待元数据在其他实例上同步完成，才能返回执行结果。如果等待超时，则返回警告信息。

+ 类型：uint32
+ 单位：秒
+ 默认值：0
+ 作用范围：Global,Session
+ 取值范围：[0,3600]
+ 是否支持在线修改生效：是

##实例组管理##

实例组管理工具包括初始化工具、配置查看工具、配置清除工具和密码修改工具。运行上述工具时，应使用数据库管理用户（安装 MySQL 实例组件时指定，默认为 sdbadmin）权限。

###初始化工具###

用户在使用实例组功能之前，需要使用实例组初始化工具 ha_inst_group_init 初始化实例组。

- **参数说明**

   | 参数 | 描述 | 是否必填 |
   | ---- | ---- | -------- |
   | --host | SequoiaDB 集群协调节点服务地址 | 否 |
   | -u, --user | 连接 SequoiaDB 集群用户的用户名 | 否 |
   | -p, --password | 连接 SequoiaDB 集群用户的密码 | 否 |
   | --key | 实例组用户密码密钥 | 否 |
   | -t, --token | 指定解密 SequoiaDB 用户密码令牌 | 否 |
   | --file | 指定 SequoiaDB 用户密码文件 | 否 |
   | --data-group | 指定 SequoiaDB 复制组，该复制组用于存储实例组中的数据 | 否 |
   | --domain | 指定实例组集合空间所属的[数据域][domain] | 否 |
   | --verbose | 输出工具的日志信息 | 否 |
   | -?, --help | 返回详细的帮助说明 | 否 |
   | --usage | 返回简要的帮助说明 | 否 |

- **使用说明**

   ```lang-text
   ha_inst_group_init [-?] [-u USER] [-p[PASSWORD]] [-t TOKEN]
            [--host=HOST] [--user=USER] [--password[=PASSWORD]] [--key=KEY]
            [--token=TOKEN] [--file=FILE] [--verbose] [--data-group=NAME]
            [--domain=DOMAIN] [--help] [--usage] inst_group_name
   ```
 
   初始化一个名为“sql_group”的实例组，同时指定实例组集合空间所属的[数据域][domain]为“domain1”
 
   ```lang-bash
   $ ha_inst_group_init sql_group --domain=domain1
   ```

###配置查看工具###

ha_inst_group_list 工具用于查看 SQL 实例的配置信息，包括实例组名、实例 ID、主机名、实例服务端口和实例类型。

- **参数说明**

   | 参数 | 描述 | 是否必填 |
   | ---- | ---- | -------- |
   | --host | SequoiaDB 集群协调节点服务地址 | 否 |
   | -u, --user | 连接 SequoiaDB 集群用户的用户名 | 否 |
   | --name | 指定要查看的实例组名称 | 否 |
   | -p, --password | 连接 SequoiaDB 集群用户的密码 | 否 |
   | -t, --token | 指定解密 SequoiaDB 用户密码令牌 | 否 |
   | --file | 指定 SequoiaDB 用户密码文件 | 否 |
   | -?, --help | 返回详细的帮助说明 | 否 |
   | --usage | 返回简要的帮助说明 | 否 |

- **使用说明**

   ```lang-text
   ha_inst_group_list [-?] [-u USER] [-p[PASSWORD]] [-t TOKEN]
              [--host=HOST] [--user=USER] [--name=INST_GROUP_NAME]
              [--password[=PASSWORD]] [--token=TOKEN] [--file=FILE] [--help]
              [--usage]
   ```
 
   * 查看所有实例的配置信息
 
     ```lang-bash
     $ ha_inst_group_list
     ```

     输出示例结果如下：
    
     ```lang-text
     InstanceGroupName    InstanceID    HostName     SvcName  DBType    DataGroup
     group1               129147        sdbserver    3309     mariadb   -
     group1               129148        sdbserver    3310     mariadb   -
     group2               129135        sdbserver    3306     mysql     -
     group2               129136        sdbserver    3307     mysql     -
     group3               129149        sdbserver    3330     mysql     -
     group3               129150        sdbserver    3331     mysql     -
     ```
 
   * 查看实例组 group2 中所有实例的配置信息
 
     ```lang-bash
     $ ha_inst_group_list --name group2
     ```
     
     输出结果如下：
     
     ```lang-text
     InstanceGroupName    InstanceID    HostName     SvcName  DBType    DataGroup
     group2               129135        sdbserver    3306     mysql     -
     group2               129136        sdbserver    3307     mysql     -
     ```
 
###配置清除工具###

ha_inst_group_clear 工具用于清除实例组或者实例的配置信息。

- **参数说明**

   | 参数 | 描述 | 是否必填 |
   | ---- | ---- | -------- |
   | --host | SequoiaDB 集群协调节点服务地址 | 否 |
   | -u, --user | 连接 SequoiaDB 用户的用户名 | 否 |
   | -p, --password | 连接 SequoiaDB 用户的密码 | 否 |
   | --force | 直接删除配置，不提示用户确认，默认为 false | 否 |
   | -t, --token | 指定解密 SequoiaDB 用户密码令牌 | 否 |
   | --file | 指定 SequoiaDB 用户密码文件 | 否 |
   | --inst_id | 通过实例 ID 指定要删除的实例 | 否 |
   | --inst_host | 通过实例的服务地址指定要删除的实例 | 否 |
   | -?, --help | 返回详细的帮助说明 | 否 |
   | --usage | 返回简要的帮助说明 | 否 |

- **使用说明**

   ```lang-text
   ha_inst_group_clear [-?] [-u USER] [-p[PASSWORD]] [-t TOKEN]
              [--host=HOST] [--user=USER] [--password[=PASSWORD]] [--force]
              [--token=TOKEN] [--file=FILE] [--inst_id=INST_ID]
              [--inst_host=INST_HOST] [--help] [--usage] inst_group_name
   ```
 
   * 清除实例组 group1 中所有实例的配置信息
 
     ```lang-bash
     $ ha_inst_group_clear group1
     ```

     提示用户是否真的需要清除实例组的配置信息
     
     ```lang-text
     Do you really want to clear instance group 'group1' [y/N]? y
     Note: clearing instance group configuration will not delete SQL instances. please use instance management tool to delete them
     Info: completed cleanup of instance group 'group1'
     ```

   * 清除实例组后，再次查看所有实例的配置信息

     ```lang-bash
     $ ha_inst_group_list
     ```
     
     输出结果如下：
     
     ```lang-text
     InstanceGroupName    InstanceID    HostName     SvcName  DBType    DataGroup
     group2               129135        sdbserver    3306     mysql     -
     group2               129136        sdbserver    3307     mysql     -
     group3               129149        sdbserver    3330     mysql     -
     group3               129150        sdbserver    3331     mysql     -
     ```
 
   * 清除实例组 group2 中服务端口为 3306 的实例配置

     ```lang-bash
     $ ha_inst_group_clear group2 --inst_host="sdbserver:3306" --force
     ```
     
     输出结果如下： 
     
     ```lang-text
     Note: clearing instance group configuration will not delete SQL instances. please use instance management tool to delete them
     Info: completed cleanup of instance 'sdbserver:3306'
     ```
 
   * 清除实例配置后，再次查看所有实例的配置信息

     ```lang-bash
     $ ha_inst_group_list
     ```
    
     输出结果如下：
    
     ```lang-text
     InstanceGroupName    InstanceID    HostName     SvcName  DBType    DataGroup
     group2               129136        sdbserver    3307     mysql     -
     group3               129149        sdbserver    3330     mysql     -
     group3               129150        sdbserver    3331     mysql     -
     ```

> **Note：**
>
> 清除实例组或实例配置并不会删除实例，删除实例需要用户手动使用 sdb_mysql_ctl 命令完成。
 
###密码修改工具###

ha_inst_group_chpass 工具用于修改配置表中的密码信息。实例组初始化后，会自动生成实例组用户信息并写入配置表。如果该用户的密码被修改，需使用 ha_inst_group_chpass 工具同步配置表中的信息，否则实例组将无法添加新的实例。

- **参数说明**

   | 参数 | 描述 | 是否必填 |
   | ---- | ---- | -------- |
   | --host | SequoiaDB 集群协调节点服务地址 | 否 |
   | -u, --user | 连接 SequoiaDB 集群用户的用户名 | 否 |
   | -p, --password | 连接 SequoiaDB 集群用户的密码 | 否 |
   | -s, --new_pass | 实例组用户密码 | 否 |
   | --key | 实例组用户密码密钥 | 否 |
   | -t, --token | 指定解密 SequoiaDB 用户密码令牌 | 否 |
   | --file | 指定 SequoiaDB 用户密码文件 | 否 |
   | --verbose | 输出工具的日志信息 | 否 |
   | -?, --help | 返回详细的帮助说明 | 否 |
   | --usage | 返回简要的帮助说明 | 否 |

- **使用说明**

   ```lang-text
   ha_inst_group_chpass [-?] [-u USER] [-p [PASSWORD]] [-s [PASSWORD]] [-t TOKEN]
              [--host=HOST] [--user=USER] [--password[=PASSWORD]] [--new_pass=[PASSWORD]] [--key=KEY]
              [--token=TOKEN] [--file=FILE] [--verbose] [--help] [--usage]
              inst_group_name
   ```

   1. 连接实例 inst1，该实例所属实例组 sql_group

     ```lang-bash
     $ mysql --socket=/opt/sequoiasql/mysql/database/3206/mysqld.sock -u root
     ```

   2. 修改实例组用户密码

     ```lang-sql
     mysql> select user from mysql.user;
     +----------------------------------+
     | User                             |
     +----------------------------------+
     | HAInstanceGroup_sql_group_D6965A |
     | root                             |
     | sdbadmin                         |
     +----------------------------------+
     mysql> ALTER USER 'HAInstanceGroup_sql_group_D6965A'@'%' IDENTIFIED BY 'sdbadmin';
     Query OK, 0 rows affected (0.01 sec)
     ```

   3. 同步配置表中的密码信息

     ```lang-bash
     $ ha_inst_group_chpass sql_group -s sdbadmin
     ```

[^_^]:
    本文使用到的所有连接及引用
[instance_group]:images/Database_Instance/Relational_Instance/MySQL_Instance/Installation/instance_group.png
[sdb_mysql_ctl]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/sdb_mysql_ctl.md
[sdb_snap_configs]:manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md
[domain]:manual/Distributed_Engine/Architecture/domain.md