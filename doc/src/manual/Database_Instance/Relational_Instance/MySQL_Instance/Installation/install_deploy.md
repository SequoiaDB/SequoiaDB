[^_^]:
    MySQL 实例-安装部署

用户使用 MySQL 实例组件前需要对 MySQL 实例组件进行安装和部署。

##安装 MySQL 实例组件##

###安装前准备###

- 使用 root 用户权限来安装 MySQL 实例组件
- 检查 MySQL 实例组件产品包是否与 SequoiaDB 版本一致 
- 如需图形界面安装，应确保 X Server 服务处于运行状态

###安装步骤###

下述安装过程将使用名称为 `sequoiasql-mysql-{version}-linux_x86_64-enterprise-installer.run` 的 MySQL 实例组件产品包为示例。

> **Note:**
>
> - 用户在安装过程中若输入有误，可按 ctrl+退格键进行删除。
> - 安装步骤以命令行方式进行介绍，若使用图形界面进行安装，可根据图形向导提示完成。

1. 以 root 用户登陆目标主机，解压 MySQL 实例组件产品包，并为解压得到的 `sequoiasql-mysql-{version}-linux_x86_64-enterprise-installer.run` 安装包赋可执行权限

    ```lang-bash
    # chmod u+x sequoiasql-mysql-{version}-linux_x86_64-enterprise-installer.run
    ```
   
2. 使用 root 用户运行 `sequoiasql-mysql-{version}-linux_x86_64-enterprise-installer.run` 包  
  
    ```lang-bash
    # ./sequoiasql-mysql-{version}-linux_x86_64-enterprise-installer.run --mode text
    ```
    
    > **Note:**   
    >
    > 执行安装包时不添加参数--mode，则进入图形界面安装模式

3. 提示选择向导语言，输入2，选择中文

    ```lang-text
    Language Selection
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] :2
    ```

4. 提示指定 MySQL 安装路径，输入回车，选择默认安装路径 `/opt/sequoiasql/mysql`；输入路径后按回车表示选择自定义路径

    ```lang-text
    ----------------------------------------------------------------------------
    由BitRock InstallBuilder评估本所建立
    
    欢迎来到 SequoiaSQL MySQL Server 安装程序
    
    ------------------------------------------------------------
    请指定 SequoiaSQL MySQL Server 将会被安装到的目录
    安装目录 /opt/sequoiasql/mysql
    ```

5. 提示配置 Linux 用户名和用户组，该用户名用于运行 MySQL 服务，输入回车，选择创建默认的用户名（sdbadmin）和用户组（sdbadmin_group）；输入用户名和用户组后按回车则表示选择自定义的用户名和用户组

    ```lang-text
    ------------------------------------------------------------
    数据库管理用户配置
    配置用于启动 SequoiaSQL-MySql 的用户名、用户组和密码
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
    设定现在已经准备将 SequoiaSQL MySQL Server 安装到您的电脑.
    您确定要继续? [Y/n]: 
    ```
  
8. 安装完成后会自动添加 sequoiasql-mysql 系统服务

    ```lang-text
    正在安装 SequoiaSQL MySQL Server 于您的电脑中，请稍候。
    安装中
    0% ______________ 50% ______________ 100%
    ########################################
    
    ------------------------------------------------------------
    安装程序已经完成安装 SequoiaSQL MySQL Server 于你的电脑中.
    ```

9. 查看 sequoiasql-mysql 服务状态

    ```lang-bash
    # service sequoiasql-mysql status
    ```
    
    系统提示 running 表示服务正在运行
    
    ```lang-text
    ● sequoiasql-mysql.service - SequoiaSQL-MySQL Server
    Loaded: loaded (/lib/systemd/system/sequoiasql-mysql.service; enabled; vendor preset: enabled)
    Active: active (running) since 五 2020-07-17 18:24:33 CST; 1 weeks 4 days ago
    ```

##部署 MySQL 实例组件##

用户需要通过 [sdb_mysql_ctl 工具][sdb_mysql_ctl]部署 MySQL 实例组件。

1. 切换用户和目录

    ```lang-bash
    # su - sdbadmin
    $ cd /opt/sequoiasql/mysql
    ```

2. 添加实例

    指定实例名为 myinst，该实例名映射相应的数据目录和日志路径，实例默认端口号为 3306（用户可以根据自己需要指定不同的实例名）
    
    ```lang-bash
    $ bin/sdb_mysql_ctl addinst myinst -D database/3306/
    ```
    
    若端口号 3306 被占用，用户可以使用 -P 参数指定实例端口号
    
    ```lang-bash
    $ bin/sdb_mysql_ctl addinst myinst -D database/3316/ -P 3316
    ```

    > **Note:**
    >
    > 用户添加的新实例会自动加入 sequoiasql-mysql 系统服务的管理中。

3. 查看实例状态

    ```lang-bash
    $ bin/sdb_mysql_ctl status
    ```
    
    系统提示 Run 表示实例部署完成，用户可通过 MySQL 客户端进行实例操作
    
    ```lang-text
    INSTANCE   PID        SVCNAME    SQLDATA                                 SQLLOG            
    myinst     25174      3306       /opt/sequoiasql/mysql/database/3306/    /opt/sequoiasql/mysql/myinst.log        
    Total: 1; Run: 1
    ```

##MySQL 实例组件系统服务##

安装 MySQL 实例组件时，会自动添加 sequoiasql-mysql 系统服务。该服务会在系统启动的时候自动运行。该服务是 MySQL 实例的守护进程。它能在机器启动时，自动启动相关的 MySQL 实例；它能实时重启异常退出的 MySQL 实例进程。

> **Note:**
>
> 一个安装对应一个 sequoiasql-mysql 服务，一台机器上存在多个安装时，系统服务名为 sequoiasql-mysql[i]，i 为小于 50 的数值或者为空。

[^_^]:
     本文使用的所有引用及链接
[sdb_mysql_ctl]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Maintainance/sdb_mysql_ctl.md