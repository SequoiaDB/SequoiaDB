[^_^]:
    PostgreSQL 实例-实例管理

sdb_pg_ctl 是 PostgreSQL 实例组件的管理工具。用户通过 sdb_pg_ctl 既可以初始化、启动和停止实例，也可以修改实例的引擎配置参数。

##参数说明##

sdb_pg_ctl 支持如下参数：

| 参数名 | 描述 |
| ------ | ---- |
| -h     | 获取帮助信息 |
| -v     | 获取版本信息 |
| -D     | 指定数据存储路径 |
| -l     | 指定日志文件，默认在安装路径下，与实例名同名 |
| -p     | 指定 PostgreSQL 服务的监听端口，默认为 5432  |
| -a     | 指定客户端最大连接数<br>该参数对应配置文件 `postgresql.conf` 中的参数 max_connections |
| -e     | 指定引擎的日志级别<br>该参数对应配置文件 `postgresql.conf` 中的参数 log_min_messages  |
| -c     | 指定 Shell 的日志级别<br>该参数对应配置文件 `postgresql.conf` 中的参数 client_min_messages | 
| -f     | 指定查询时每次获取的记录数 |
| -o     | 指定传递给 postgres 或 initdb 命令的选项，具体选项说明可在安装目录下通过 `bin/pg_ctl --help` 或 `bin/initdb --help` 查看 <br> 在创建实例时，-o 需填入 initdb 相关选项；在启动实例时，-o 需填入 pg_ctl 相关选项 |
| -m     | 指定 PostgreSQL 服务器的关闭模式，默认值为 fast，取值如下：<br>smart：等待所有连接断开后关闭服务器<br>fast：快速关闭服务器，将回滚所有未提交的事务并强行断开客户端 <br>immediate：立即关闭服务器，将终止所有服务器进程并直接退出；该关闭方式会导致下次重启时进入实例恢复状态   |
| --addtosvc=BOOL | 是否将创建的实例添加至 [sequoiasql-postgresql 系统服务][service]，默认为 true，添加至系统服务 |
| --skip-initdb | 将用户通过 `bin/initdb` 创建的实例添加至 sequoiasql-postgresql 系统服务时，跳过 initdb 操作 |
| --print | 打印日志信息 |
| --baklog | 删除实例时备份日志文件 |
| --force | 强制删除指定实例 |

##使用说明##

运行 sdb_pg_ctl 工具时，应使用数据库管理用户（安装 PostgreSQL 实例组件时指定，默认为 sdbadmin）权限。

###管理实例###

- 创建实例
   
    sdb_pg_ctl addinst \<INSTNAME\> \<-D DATADIR\> [-l LOGFILE] [--print] [-p PORT] [-o "OPTIONS"] [--skip-initdb] [--addtosvc=BOOL]

    创建名为“myinst”的实例，并指定数据存储路径为 `/opt/sequoiasql/postgresql/database/5432`

    ```lang-bash
    $ sdb_pg_ctl addinst myinst -D /opt/sequoiasql/postgresql/database/5432
    ```

- 启动实例

    sdb_pg_ctl start \<INSTNAME\> [--print] [-o "OPTIONS"]

    启动名为“myinst”的实例

    ```lang-bash
    $ sdb_pg_ctl start myinst
    ```

- 启动所有实例

    ```lang-bash
    $ sdb_pg_ctl startall
    ```
	
- 查看实例状态

    sdb_pg_ctl status [INSTNAME]

    查看所有实例的状态

    ```lang-bash
    $ sdb_pg_ctl status
    ```

- 查看所有已创建的实例

    ```lang-bash
    $ sdb_pg_ctl listinst
    ```

- 重启实例

    sdb_pg_ctl restart \<INSTNAME\> [-m SHUTDOWN-MODE] [--print] [-o "OPTIONS"]

    重启名为“myinst”的实例

    ```lang-bash
    $ sdb_pg_ctl restart myinst
    ```

- 停止实例

    sdb_pg_ctl stop \<INSTNAME\> [-m SHUTDOWN-MODE] [--print]

    停止名为“myinst”的实例，并指定服务器的关闭模式为 smart

    ```lang-bash
    $ sdb_pg_ctl stop myinst -m smart
    ```

- 停止所有实例

    sdb_pg_ctl stopall [-m SHUTDOWN-MODE] [--print]
 
    停止所有实例，并指定服务器的关闭模式为 smart

    ```lang-bash
    $ sdb_pg_ctl stopall -m smart
    ```

- 删除实例

    sdb_pg_ctl delinst \<INSTNAME\> [--baklog] [--force]

    删除名为“myinst”的实例，并备份该实例日志文件

    ```lang-bash
    $ sdb_pg_ctl delinst myinst --baklog
    ```

- 重载实例配置

    sdb_pg_ctl reload \<INSTNAME\> [--print]

    重载名为“myinst”实例的配置

    ```lang-bash
    $ sdb_pg_ctl reload myinst
    ```

- 将实例添加至系统服务

    sdb_pg_ctl addtosvc \<INSTNAME\> [--print]

    将名为“myinst”的实例添加至 sequoiasql-postgresql 系统服务

    ```lang-bash
    $ sdb_pg_ctl addtosvc myinst
    ```    

- 将实例从系统服务中移除

    sdb_pg_ctl delfromsvc \<INSTNAME\>

    将名为“myinst”的实例从 sequoiasql-postgresql 系统服务中移除

    ```lang-bash
    $ sdb_pg_ctl delfromsvc myinst
    ```
    

###修改实例配置###

用户可通过 sdb_pg_ctl 修改指定实例的配置。

```lang-text
sdb_pg_ctl chconf <INSTNAME> [-p PORT] [-c LEVEL] [-e LEVEL] [-a MAX-CON]
							 [-f CNT]
```

**示例**

修改实例 myinst 的引擎日志级别为 notice

```lang-bash
$ sdb_pg_ctl chconf myinst -e notice
```

查看配置是否修改成功

```lang-sql
sample=# select * from pg_settings;
```



[^_^]:
    本文使用的所有引用及链接
[pgsql]:https://www.postgresql.org/docs/9.3/app-pg-ctl.html
[service]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/install_deploy.md#PostgreSQL%20实例组件系统服务