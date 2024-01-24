[^_^]:
    PostgreSQL 实例-安装部署

用户使用 PostgreSQL 实例组件前需要对 PostgreSQL 实例组件进行安装和部署。

##安装 PostgreSQL 实例组件##

###安装前准备###

- 使用 root 用户权限来安装 PostgreSQL 实例组件
- 检查 PostgreSQL 实例组件产品软件包是否与 SequoiaDB 版本一致 
- 如果需要图形界面模式安装，应确保 X Server 服务正在运行

###安装步骤###

下述安装过程将使用名称为 `sequoiasql-postgresql-{version}-x86_64-enterprise-installer.run` 的 PostgreSQL 实例组件产品包为示例。

> **Note:**
>
> - 用户在安装过程中若输入有误，可按ctrl+退格键进行删除。
> - 安装步骤以命令行方式进行介绍，若使用图形界面进行安装，可根据图形向导提示完成。

1. 以 root 用户登陆目标主机，解压 PostgreSQL 实例组件产品包，并为解压得到的 `sequoiasql-postgresql-{version}-x86_64-enterprise-installer.run` 安装包赋可执行权限

    ```lang-bash
    # chmod u+x sequoiasql-postgresql-{version}-x86_64-enterprise-installer.run
    ```

2. 使用 root 用户运行 `sequoiasql-postgresql-{version}-x86_64-enterprise-installer.run` 包
  
    ```lang-bash
    # ./sequoiasql-postgresql-{version}-x86_64-enterprise-installer.run --mode text
    ```

    > **Note:**
    >
    > 执行安装包不添加参数--mode，则进入图形界面。  

3. 提示选择向导语言，输入2，选择中文

    ```lang-text
    Language Selection
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] :2
    ```

4. 提示指定 PostgreSQL 安装路径，输入回车，选择默认安装路径 `/opt/sequoiasql/postgresql`；输入路径后按回车则表示选择自定义路径

    ```lang-text
    ----------------------------------------------------------------------------
    由BitRock InstallBuilder评估本所建立
    
    欢迎来到 SequoiaSQL PostgreSQL Server 安装程序
    
    ------------------------------------------------------------
    请指定 SequoiaSQL PostgreSQL Server 将会被安装到的目录
    安装目录 [/opt/sequoiasql/postgresql]:
    ```

5. 提示配置 Linux 用户名和用户组，该用户名用于运行 PostgreSQL 服务，输入回车，选择创建默认的用户名（sdbadmin）和用户组（sdbadmin_group）；输入用户名和用户组后按回车则表示选择自定义的用户名和用户组

    ```lang-text
    ------------------------------------------------------------
    数据库管理用户配置
    配置用于启动 SequoiaSQL PostgreSQL 的用户名、用户组和密码
    用户名 [sdbadmin]:
    用户组 [sdbadmin_group]:
    ```

6. 提示配置刚才创建的 Linux 用户密码，输入回车，选择使用默认密码（Admin@1024）；输入密码后按回车表示选择自定义密码

    ```lang-text
    密码 [********]:
    确认密码 [********]:
    ```

7. 输入回车，确认继续

    ```lang-text
    ------------------------------------------------------------
    设定现在已经准备将 SequoiaSQL PostgreSQL Server 安装到您的电脑.
    您确定要继续? [Y/n]: 
    ```
  
8. 安装完成后会自动添加 sequoiasql-postgresql 系统服务

    ```lang-text
    正在安装 SequoiaSQL PostgreSQL Server 于您的电脑中，请稍候。
    安装中
    0% ______________ 50% ______________ 100%
    ########################################
    添加了系统服务: sequoiasql-postgresql.
    #
    ------------------------------------------------------------
    安装程序已经完成安装 SequoiaSQL PostgreSQL Server 于你的电脑中.
    ```

9. 查看 sequoiasql-postgresql 服务状态

    ```lang-bash
    # service sequoiasql-postgresql status
    ```
    
    系统提示 running 表示服务正在运行
    
    ```lang-text
    ● sequoiasql-postgresql.service - SequoiaSQL-PostgreSQL Daemon
     Loaded: loaded (/lib/systemd/system/sequoiasql-postgresql.service; enabled; vendor preset: enabled)
     Active: active (running) since 四 2020-08-27 15:55:40 CST; 1 months 16 days ago
    ```

##部署 PostgreSQL 实例组件##

用户需要通过 sdb_pg_ctl 工具部署 PostgreSQL 实例组件。

1. 切换用户和目录

    ```lang-bash
    # su - sdbadmin
    $ cd /opt/sequoiasql/postgresql
    ```

2. 添加实例

    指定实例名为 myinst，该实例名映射相应的数据目录和日志路径，实例默认端口号为 5432（用户可根据需要指定不同的实例名）

    ```lang-bash
    $ bin/sdb_pg_ctl addinst myinst -D database/5432/
    ```
    
    若端口号 5432 被占用，用户可以使用 -p 参数指定实例端口号
    
    ```lang-bash
    $ bin/sdb_pg_ctl addinst myinst -D database/5442/ -p 5442
    ```

3. 启动实例进程

    ```lang-bash
    $ bin/sdb_pg_ctl start myinst
    ```

4. 查看实例状态

    ```lang-bash
    $ bin/sdb_pg_ctl status
    ```
    
    系统提示 Run 表示实例部署完成，用户可通过 PostgreSQL Shell 进行实例操作
    
    ```lang-text
    NAME       PGDATA                                       PGLOG
    myinst     /opt/sequoiasql/postgresql/database/5432/    /opt/sequoiasql/postgresql/myinst.log
    Total: 1
    ```

##sequoiasql-postgresql 系统服务##

sequoiasql-postgresql 系统服务是 PostgreSQL 实例的守护进程，会在系统启动的时候自动运行，用于保证 PostgreSQL 实例的可靠性。该服务启动时，会自动启动相关的实例。在实例进程异常退出时，sequoiasql-postgresql 服务会自动重启该实例进程。

> **Note:**  
> 
> 一个安装对应一个 sequoiasql-postgresql 服务，一台机器上存在多个安装时，系统服务名为 sequoiasql-postgresql[i]，i 为小于 50 的数值或者为空。