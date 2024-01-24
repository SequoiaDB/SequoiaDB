本文档主要介绍如何在 SAC 上部署 PostgreSQL 实例。在部署 PostgreSQL 实例之前用户需要先完成[部署包][deploy_package]操作。

##创建 PostgreSQL 实例##

创建 PostgreSQL 实例是指在已经安装了部署包的主机中创建一个 PostgreSQL 实例。

1. 进入【部署】页面，在【数据库实例】栏中点击 **创建实例** 按钮，实例类型选择 PostgreSQL 后点击 **确定** 按钮进入【配置服务】页面
![安装SequoiaSQL-PostgreSQL][add_pgsql_1]

2. 在【配置服务】页面，可以修改 PostgreSQL 的配置，配置项留空表示默认值，点击 **下一步** 按钮开始安装
![安装SequoiaSQL-PostgreSQL][add_pgsql_2]

 >**Note:**
 >
 > 没有安装 PostgreSQL 包的 sdbserver1 主机，会出现安装路径和系统管理员配置项：
 > - 安装路径指 PostgreSQL 包安装在主机中的路径；
 > - 系统管理员指主机的管理员账号，安装 PostgreSQL 包需要管理员权限。

3. 安装服务完成
![安装SequoiaSQL-PostgreSQL][add_pgsql_3]

> **Note：**
>  
> 用户创建 PostgreSQL 实例时自动建立一个名为 postgres 的库，建议后续操作不要删除此库。  

##添加已有 PostgreSQL 实例##

如果用户不需要创建实例，可以将已有 PostgreSQL 实例添加至 SAC。

1. 进入【部署】->【数据库实例】页面

   ![数据库实例][append_postgresql_1]

2. 点击【添加实例】->【添加已有的实例】按钮

   ![添加 PostgreSQL 实例][append_postgresql_2]

3. 实例类型选择 PostgreSQL 并输入参数后点击 **确定** 按钮

   ![添加 PostgreSQL 实例][append_postgresql_3]

   > **Note:**
   >
   > - 地址：PostgreSQL 数据库的主机名或者 IP 地址  
   > - 端口：PostgreSQL 数据库的监听端口号  
   > - 数据库用户名：选填，PostgreSQL 数据库的用户名  
   > - 数据库密码：选填，PostgreSQL 数据库的密码

4. 添加实例完成，这里可以查看 PostgreSQL 实例的配置，点击 **返回** 按钮

  ![添加 PostgreSQL 实例][append_postgresql_4]

5. 【部署】->【数据库实例】页面已经显示出 PostgreSQL 实例

  ![添加 PostgreSQL 实例][append_postgresql_5]

##添加 PostgreSQL 实例存储##

用户创建 PostgreSQL 实例或添加已有 PostgreSQL 实例后，需要添加实例存储将 PostgreSQL 实例与 SequoiaDB 巨杉数据库进行关联。

> **Note:**
>
> PostgreSQL 实例可以添加多个存储集群。

1. 进入【部署】页面，在【数据库实例】栏中点击【配置实例存储】->【添加实例存储】按钮

 ![添加实例存储][pg_sdb_1]

2. 在弹窗中填写参数后点击 **确定** 按钮
 
 ![添加实例存储][pg_sdb_2]

 ![添加实例存储][pg_sdb_3]

   > **Note:**
   >
   > - 关联名：创建关联完成后的名字，全局唯一   
   > - preferedinstance：指定 PostgreSQL 实例访问 SequoiaDB 数据节点时，优先连接哪种角色的数据节点，默认为’a’，可输入参数 m/s/a/1~7，分别表示 master/slave/anyone/node1~node7   
   > - transaction：设置 SequoiaDB 是否开启事务，默认为 off，开启为 on   
   > - 存储集群的节点：选择关联 SequoiaDB 服务的 coord 节点，默认所有 coord 节点

3. 添加实例存储成功

 ![添加实例存储][pg_sdb_4]


[^_^]:
    引用页面
[^_^]:
    TODO:
[deploy_package]:manual/SAC/Deployment/Deployment_Bystep/deploy_package.md

[add_pgsql_1]:images/SAC/Deployment/Deployment_Bystep/add_pgsql_1.png
[add_pgsql_2]:images/SAC/Deployment/Deployment_Bystep/add_pgsql_2.png
[add_pgsql_3]:images/SAC/Deployment/Deployment_Bystep/add_pgsql_3.png
[append_postgresql_1]:images/SAC/Deployment/Deployment_Bystep/append_postgresql_1.png
[append_postgresql_2]:images/SAC/Deployment/Deployment_Bystep/append_postgresql_2.png
[append_postgresql_3]:images/SAC/Deployment/Deployment_Bystep/append_postgresql_3.png
[append_postgresql_4]:images/SAC/Deployment/Deployment_Bystep/append_postgresql_4.png
[append_postgresql_5]:images/SAC/Deployment/Deployment_Bystep/append_postgresql_5.png
[pg_sdb_1]:images/SAC/Deployment/Deployment_Bystep/pg_sdb_1.png
[pg_sdb_2]:images/SAC/Deployment/Deployment_Bystep/pg_sdb_2.png
[pg_sdb_3]:images/SAC/Deployment/Deployment_Bystep/pg_sdb_3.png
[pg_sdb_4]:images/SAC/Deployment/Deployment_Bystep/pg_sdb_4.png