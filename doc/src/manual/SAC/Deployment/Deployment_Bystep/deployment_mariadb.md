本文档主要介绍如何在 SAC 上部署 MariaDB 实例。在部署 MariaDB 实例之前用户需要先完成[部署包][deploy_package]操作。

##创建 MariaDB 实例##

创建 MariaDB 实例是指在已经安装部署包的主机中创建一个 MariaDB 实例。

1. 进入【部署】页面, 在【数据库实例】栏中点击 **创建实例** 按钮，实例类型选择 MariaDB 后点击 **确定** 按钮进入【配置服务】页面
![安装SequoiaSQL-MariaDB][add_mariadb_1]

2. 在【配置服务】页面，可以修改 MariaDB 实例的配置，点击 **下一步** 按钮开始安装
![安装SequoiaSQL-MariaDB][add_mariadb_2]

 >**Note:**
 >
 > 没有安装 MariaDB 包的 sdbserver1 主机，会出现安装路径和系统管理员配置项：
 > - 安装路径指 MariaDB 包安装在主机中的路径；
 > - 系统管理员指主机的管理员账号，安装 MariaDB 包需要管理员权限。
> 
> MariaDB 账号：用于访问 MariaDB 数据库的账号，如果用户名是 root，则修改默认账号 root 的密码

3. 安装服务完成
![安装SequoiaSQL-MariaDB][add_mariadb_3]

##添加已有 MariaDB 实例##

如果用户不需要创建实例，可以将已有 MariaDB 实例添加至 SAC。

1. 进入【部署】->【数据库实例】页面

   ![数据库实例][append_mariadb_1]

2. 点击【添加实例】->【添加已有的实例】按钮

   ![添加 MariaDB 实例][append_mariadb_2]

3. 实例类型选择 MariaDB 并输入参数，点击 **确定** 按钮

   ![添加 MariaDB 实例][append_mariadb_3]

   > **Note:**
   >
   > - 地址：MariaDB 数据库的主机名或者 IP 地址
   > - 端口：MariaDB 数据库的监听端口号  
   > - 数据库用户名：MariaDB 数据库的用户名  
   > - 数据库密码：选填，MariaDB 数据库的密码

4. 添加实例完成，该页面可以查看 MariaDB 实例的配置，点击 **返回** 按钮

  ![添加 MariaDB 实例][append_mariadb_4]

5. 【部署】->【数据库实例】页面已经显示出 MariaDB 实例

  ![添加 MariaDB 实例][append_mariadb_5]

##添加 MariaDB 实例存储##

用户创建 MariaDB 实例或添加已有 MariaDB 实例后，需要添加实例存储将 MariaDB 实例与 SequoiaDB 巨杉数据库进行关联。

1. 进入【部署】页面，在【数据库实例】栏中点击【配置实例存储】->【添加实例存储】按钮
![关联服务][mariadb_sdb_1]

2. 在弹窗填写参数后点击 **确定** 按钮

 ![关联服务][mariadb_sdb_2]
 ![关联服务][mariadb_sdb_3]

   > **Note:**
   >
   > - 关联名：创建关联完成后的名字，全局唯一   
   > - preferedinstance：指定 MariaDB 实例访问 SequoiaDB 数据节点时，优先连接哪种角色的数据节点，默认为’a’，可输入参数 m/s/a/1~7，分别表示 master/slave/anyone/node1~node7   
   > - transaction：设置 SequoiaDB 是否开启事务，默认为 off，开启为 on   
   > - 存储集群的节点：选择关联 SequoiaDB 服务的 coord 节点，默认所有 coord 节点

3. 添加实例存储成功
![关联服务][mariadb_sdb_4]

[^_^]:
    本文使用的所有引用和链接 
[deploy_package]:manual/SAC/Deployment/Deployment_Bystep/deploy_package.md
[add_mariadb_1]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_1.png
[add_mariadb_2]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_2.png
[add_mariadb_3]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_3.png
[append_mariadb_1]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_4.png
[append_mariadb_2]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_5.png
[append_mariadb_3]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_6.png
[append_mariadb_4]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_7.png
[append_mariadb_5]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_8.png
[mariadb_sdb_1]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_9.png
[mariadb_sdb_2]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_10.png
[mariadb_sdb_3]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_11.png
[mariadb_sdb_4]:images/SAC/Deployment/Deployment_Bystep/add_mariadb_12.png