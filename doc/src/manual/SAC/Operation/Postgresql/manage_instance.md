本文档将介绍 PostgreSQL 实例的查看节点详细配置、修改配置、同步配置及设置鉴权操作。

##查看节点详细配置##

用户通过点击左侧导航 **配置** 选择 PostgreSQL 实例点击进入配置页面，点击 **查看详细配置** 按钮查看当前实例的详细配置。
![查看配置][pgsql_config_2]

##修改配置##

用户在修改 PostgreSQL 实例配置页面可以查看当前服务的配置以及对配置进行在线修改。

点击 **修改配置** 按钮，填写需要修改的配置后点击 **确定** 按钮
![修改配置][pgsql_config_3]

如果修改的配置项中有需要重启生效的项，则该配置项会在提示窗口中说明，需要点击 **重启节点** 按钮进行重启服务
![重启配置][pgsql_config_4]

##同步配置##

同步配置可以把 PostgreSQL 数据库实例配置同步到 SAC 中。

1. 进入【部署】->【分布式存储】页面，点击【存储集群操作】->【同步配置】按钮

   ![同步配置][sync_1]

2. 选择要同步配置的 PostgreSQL 数据库实例，点击 **确定** 按钮

   ![同步配置][sync_2]

3. 同步完成

   ![同步配置][sync_3]

##设置鉴权##

用户可以通过修改配置文件或 SAC 设置鉴权

- 通过配置文件设置鉴权

   > **Note：**  
   >
   >- 为了保证安全，生产环境请不要使用简单密码。  
   >- PostgreSQL 默认用 sdbadmin 账号访问，这跟 SAC 创建集群的用户名相关

   1. 进入 PostgreSQL Shell

     ```lang-bash
     $ su sdbadmin
     $ /opt/sequoiasql/postgresql/bin/psql -p 5432 postgres
     ```
  
   2. 创建新用户，用户名“sac”，密码“123”

     ```lang-sql
     postgres=# create user sac with password '123';
     ```

   3. 修改 PostgreSQL 数据库实例的客户端认证配置文件 `pg_hba.conf`

     ```lang-bash
     $ vi /opt/sequoiasql/postgresql/database/5432/pg_hba.conf
     ```

     配置内容修改前：

     ```lang-text
     ...
     # TYPE  DATABASE        USER            ADDRESS                 METHOD

     # "local" is for Unix domain socket connections only
     local   all             all                                     trust
     # IPv4 local connections:
     host    all             all             127.0.0.1/32            trust
     host    all             all             0.0.0.0/0               trust
     # IPv6 local connections:
     host    all             all             ::1/128                 trust
     ...
     ```

     配置内容修改后，表示使用 TCP 连接的需要携带 md5 算法加密的密码认证：

     ```lang-text
     ...
     # TYPE  DATABASE        USER            ADDRESS                 METHOD

     # "local" is for Unix domain socket connections only
     local   all             all                                     trust
     # IPv4 local connections:
     host    all             all             127.0.0.1/32            trust
     host    all             all             0.0.0.0/0               md5
     # IPv6 local connections:
     host    all             all             ::1/128                 trust
     ...
     ```

   4. 重新加载 PostgreSQL 数据库实例配置

     ```lang-bash
     /opt/sequoiasql/postgresql/bin/sdb_pg_ctl reload PostgreSQLInstance1
     ```

- 通过 SAC 设置鉴权

   1. 进入【部署】->【数据库实例】页面

     ![设置鉴权][auth_1]

   2. 点击所选数据库实例的 **鉴权** 按钮，在弹窗输入用户名和密码后点击 **确定** 按钮

     ![设置鉴权][auth_2]

   3. 进入【数据】->【数据库实例】页面，访问 PostgreSQL 数据库实例的信息

     ![设置鉴权][auth_3]





[^_^]:
     本文使用的所有引用及链接
[pgsql_config_2]:images/SAC/Operation/Postgresql/pgsql_config_2.png
[pgsql_config_3]:images/SAC/Operation/Postgresql/pgsql_config_3.png
[pgsql_config_4]:images/SAC/Operation/Postgresql/pgsql_config_4.png
[sync_1]:images/SAC/Operation/Postgresql/sync_1.png
[sync_2]:images/SAC/Operation/Postgresql/sync_2.png
[sync_3]:images/SAC/Operation/Postgresql/sync_3.png
[auth_1]:images/SAC/Operation/Postgresql/auth_1.png
[auth_2]:images/SAC/Operation/Postgresql/auth_2.png
[auth_3]:images/SAC/Operation/Postgresql/auth_3.png



