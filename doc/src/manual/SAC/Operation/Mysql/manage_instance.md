本文档主要介绍 MySQL 实例的修改配置、同步配置及设置鉴权操作。

##修改配置##

用户通过左侧导航【配置】选择 MySQL 实例点击进入配置页面，可以查看当前服务的配置以及对配置进行在线修改。
![配置][mysql_config_1]

###多项修改配置###

点击 **修改配置** 按钮，根据需要修改配置项，修改完毕后点击 **确定** 按钮完成修改
![查看配置][mysql_config_3]

###单项修改配置###

1. 通过表格中的筛选框查找需要修改的配置项
![修改配置][mysql_config_4]  

2. 点击所选配置项，在弹窗中填写需要修改的值后点击 **确定** 按钮
![修改配置][mysql_config_5] 

3. 修改完成
![修改配置][mysql_config_6]

##同步配置##

同步配置可以把 MySQL 数据库实例配置同步到 SAC 中。

1. 进入【部署】->【分布式存储】页面，点击【存储集群操作】->【同步配置】按钮

   ![同步配置][sync_1]

2. 选择需要同步配置的 MySQL 数据库实例，点击 **确定** 按钮

   ![同步配置][sync_2]

3. 同步完成

   ![同步配置][sync_3]

##设置鉴权##

用户可以通过 MySQL 客户端或 SAC 设置鉴权。

- 通过 MySQL 客户端设置鉴权

   1. 进入 MySQL 客户端

     ```lang-bash
     $ /opt/sequoiasql/mysql/bin/mysql -h 127.0.0.1 -P 3306 -u root
     ```

   2. 创建新用户，用户名为“sac”，密码为“123”

      ```lang-sql
      mysql> grant all privileges on *.* to "sac"@"%" identified by "123";
      mysql> flush privileges;
      ```

      如下为修改已存在用户的密码，如将 root 用户的密码修改为“123”：

      ```lang-sql
      mysql> update mysql.user set authentication_string = password("123") where user = "root";
      mysql> flush privileges;
      ```

   > **Note:**  
   >
   > 为了保证安全，生产环境请不要使用简单密码。

- 通过 SAC 设置鉴权

   1. 进入【部署】->【数据库实例】页面

     ![设置鉴权][auth_1]

   2. 点击所选数据库实例的【鉴权】->【修改鉴权】按钮

     ![设置鉴权][auth_2]

   3. 在弹窗输入用户名和密码后，点击 **确定** 按钮完成修改

     ![设置鉴权][auth_3]




[^_^]:
     本文使用的所有引用和链接
[mysql_config_1]:images/SAC/Operation/Mysql/mysql_config_1.png
[mysql_config_3]:images/SAC/Operation/Mysql/mysql_config_3.png
[mysql_config_4]:images/SAC/Operation/Mysql/mysql_config_4.png
[mysql_config_5]:images/SAC/Operation/Mysql/mysql_config_5.png
[mysql_config_6]:images/SAC/Operation/Mysql/mysql_config_6.png
[sync_1]:images/SAC/Operation/Mysql/sync_1.png
[sync_2]:images/SAC/Operation/Mysql/sync_2.png
[sync_3]:images/SAC/Operation/Mysql/sync_3.png
[auth_1]:images/SAC/Operation/Mysql/auth_1.png
[auth_2]:images/SAC/Operation/Mysql/auth_2.png
[auth_3]:images/SAC/Operation/Mysql/auth_3.png
