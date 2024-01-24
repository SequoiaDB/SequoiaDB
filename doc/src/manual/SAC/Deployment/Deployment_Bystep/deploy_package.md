部署包是将指定的包部署到指定的主机中，目前支持部署 PostgreSQL 包和 MySQL 包。下述演示步骤以 PostgreSQL 包为例。 

> **Note:**  
>
> PostgreSQL 和 MySQL 的 run 包在 SequoiaDB 安装压缩包的根目录下，部署包之前需要将 run 包放在 SAC 所在主机的安装路径 packet 路径下，默认路径为 `/opt/sequoiadb/packet`。

1. 在【部署】页面勾选需要进行操作的主机，点击【主机操作】->【部署包】按钮进入配置页面
![部署包][deploy_package_1]

2. 填写好参数之后，点击 **下一步** 按钮进行安装

   ![部署包][deploy_package_2]

   > **Note:**  
   >
   > 强制安装： 
   >- 选择 false 时，会忽略已经安装的主机  
   >- 选择 true 时，会在选择的主机上重新安装，并且强制重启的对应服务
   >
   > 用户名：支持 root 和 sudo 权限的用户

3. 安装完成
![部署包][deploy_package_3]

4. 点击 **完成** 按钮，回到【部署】页面，查看主机，可以看见刚刚安装的两台主机已经支持 PostgreSQL
![部署包][deploy_package_4]



[^_^]:
    本文使用的所有链接及引用
[deploy_package_1]:images/SAC/Deployment/Deployment_Bystep/deploy_package_1.png
[deploy_package_2]:images/SAC/Deployment/Deployment_Bystep/deploy_package_2.png
[deploy_package_3]:images/SAC/Deployment/Deployment_Bystep/deploy_package_3.png
[deploy_package_4]:images/SAC/Deployment/Deployment_Bystep/deploy_package_4.png
