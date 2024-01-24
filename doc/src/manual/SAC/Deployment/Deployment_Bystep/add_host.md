1. 在【部署】->【主机】页面点击 **添加主机** 按钮
![添加主机][scan_host]

2. 在【IP地址/主机名】输入要扫描主机的 IP 地址或主机名

    ![扫描主机][scan_host_1]

   扫描主机支持以下输入方式：

   * 普通格式
     ![扫描主机][scan_host_2]

   * 区间格式
     ![扫描主机][scan_host_3]

   * 逗号分隔
      ![扫描主机][scan_host_4]

   * 换行分隔
      ![扫描主机][scan_host_5]

3. 【用户名】必须是 root，安装需要 root 权限并输入 root 密码

   * 设置 Linux 系统 root 密码

     ```lang-bash
     # sudo passwd root
     ```

   * 配置 SSH 允许 root 使用密码登陆

     修改配置文件 

      ```lang-bash
      # vim /etc/ssh/sshd_config
      ```

      修改以下配置项的值为 yes

      ```lang-text
      PermitRootLogin yes
      PasswordAuthentication yes
      ```

       重启 SSH 服务
   
       ```lang-bash
       # service sshd restart
       ```


4. 【SSH 端口】根据实际端口修改（默认为 22），代理端口默认为 11790

5. 点击 **扫描** 按钮，扫描成功后点击 **下一步** 按钮
![扫描主机][scan_host_6]

6. 系统会自动检查主机是否符合安装服务的要求，符合条件的主机在左侧【主机列表】都会自动勾选
![添加主机][add_host_1]

 > **Note:**
 > 
 > 演示环境 sdbserver1 主机为提供 OM 服务，默认不会选择，需要手工勾选。

7. 在【主机配置】页面可以选择磁盘，容量不足和网络盘禁止选择，符合条件的磁盘都会默认选中
![添加主机][add_host_2]

 > **Note:**
 > 
 > 选中的磁盘将会在配置服务时自动分配服务的节点到磁盘上，如果用户不希望节点安装在某个磁盘上，可以取消选中该磁盘。

8. 点击 **下一步** 按钮开始安装

9. 等待安装完成
![安装主机][install_host_1]

10. 安装完成
![安装主机][install_host_2]

11. 部署
   * 如果是【一键部署】，点击底部的 **下一步** 按钮，进入【服务配置】页面，开始配置 SequoiaDB 服务，[点击查看][add_sdb_module]
    ![安装主机][install_host_3]

   * 如果是部署主机，点击底部的 **完成** 按钮，回到【部署】页面进行[存储集群部署][add_sdb_module]


[^_^]:
    本文使用的所有引用和链接
[scan_host]:images/SAC/Deployment/Deployment_Bystep/scan_host.png
[scan_host_1]:images/SAC/Deployment/Deployment_Bystep/scan_host_1.png
[scan_host_2]:images/SAC/Deployment/Deployment_Bystep/scan_host_2.png
[scan_host_3]:images/SAC/Deployment/Deployment_Bystep/scan_host_3.png
[scan_host_4]:images/SAC/Deployment/Deployment_Bystep/scan_host_4.png
[scan_host_5]:images/SAC/Deployment/Deployment_Bystep/scan_host_5.png
[scan_host_6]:images/SAC/Deployment/Deployment_Bystep/scan_host_6.png
[add_host_1]:images/SAC/Deployment/Deployment_Bystep/add_host_1.png
[add_host_2]:images/SAC/Deployment/Deployment_Bystep/add_host_2.png
[install_host_1]:images/SAC/Deployment/Deployment_Bystep/install_host_1.png
[install_host_2]:images/SAC/Deployment/Deployment_Bystep/install_host_2.png
[install_host_3]:images/SAC/Deployment/Deployment_Bystep/install_host_3.png

[add_sdb_module]:manual/SAC/Deployment/Deployment_Bystep/add_sdb_module.md