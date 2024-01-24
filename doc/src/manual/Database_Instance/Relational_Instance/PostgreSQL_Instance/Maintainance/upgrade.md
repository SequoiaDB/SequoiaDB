[^_^]:
     PostgreSQL 实例-升级

升级操作用于将 PostgreSQL 实例组件从低版本升级至高版本。升级后不会改动任何配置和数据，同版本间也可进行升级。

> **Note:**
>
> 升级操作不支持将 PostgreSQL 实例组件从高版本降级为低版本。

下述以安装包 `sequoiasql-postgresql-5.0.2-x86_64-installer.run` 为例升级 PostgreSQL 实例组件，升级步骤如下：

1. 使用 root 用户指定升级参数进行升级

    ```lang-bash
    # ./sequoiasql-postgresql-5.0.2-x86_64-installer.run --mode text --installmode upgrade
    ```

2. 提示选择向导语言，输入2，选择中文

    ```lang-text
    Language Selection
    
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] : 2
    ```

3. 输入回车，确认继续

    ```lang-text
    ----------------------------------------------------------------------------
    由BitRock InstallBuilder评估本所建立
    
    欢迎来到 SequoiaSQL PostgreSQL Server 安装程序
    
    ----------------------------------------------------------------------------
    设定现在已经准备将 SequoiaSQL PostgreSQL Server 安装到您的电脑.
    
    您确定要继续? [Y/n]: 
    ```

4. 升级完成

    ```lang-text
    ----------------------------------------------------------------------------
    正在安装 SequoiaSQL PostgreSQL Server 于您的电脑中，请稍候.
    
     安装中
     0% ______________ 50% ______________ 100%
     开始升级
    **************************  检查列表 *************************************
    检查：在/etc/default/sequoiasql-postgresql中获取用户名 ...... ok
    检查：安装目录/opt/sequoiasql/postgresql不应该为空 ...... ok
    检查：旧版本 3.2.7 与新版本 5.0.2 兼容 ...... ok
    检查：磁盘空间足够 ...... ok
    检查：umask配置 ...... ok
    检查：/opt/sequoiasql/postgresql/bin/sdb_sql_ctl 存在 ......ok
    检查：用户sdbadmin存在，并获取用户组 ...... ok
    检查：相关进程已停止 ...... ok
    #########################################
    
    ----------------------------------------------------------------------------
    安装程序已经完成安装 SequoiaSQL PostgreSQL Server 于你的电脑中.
    ```


5. 查看当前版本

    ```lang-bash
    # /opt/sequoiasql/postgresql/bin/sdb_pg_ctl --version
    ```