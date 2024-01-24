本文档主要介绍如何安装 OM 服务及登录 SAC。SequoiaDB 巨杉数据库的可视化安装在多台服务器的情况下，只需要选择一台机器安装 OM 服务（SequoiaDB 管理中心进程），即可通过网页连接 OM 进行可视化安装部署集群。

##安装##

###安装说明###

- 安装前需确保主机已安装 SequoiaDB 巨杉数据库。
- 安装过程需要使用数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin）。

###安装步骤###

下述以 SequoiaDB 安装目录 `/opt/sequoiadb`、SequoiaDB 和 Sequoiasql 产品包所在目录 `/opt` 为例，介绍 OM 服务安装和[部署包][packet]的步骤。

1. 切换至 SequoiaDB 安装目录

    ```lang-bash
    $ cd /opt/sequoiadb
    ```

2. 执行安装脚本 `install_om.sh`

    ```lang-bash
    $ ./install_om.sh
    ```

3. 提示配置 OM 服务端口号，输入回车，选择默认端口 11780；输入端口号后按回车表示选择自定义端口

    ```lang-text
    SequoiaDB om service port [11780]:
    ```

4. 提示配置 SAC 端口号，输入回车，选择默认端口 8000；输入端口号后按回车表示选择自定义端口

    ```lang-text
    SequoiaDB SAC port [8000]:
    ```

5. OM 服务安装完成

    ```lang-text
    Succeed to create om
    ```

6. 提示是否部署 SQL 产品包，输入 y 后按回车，可进行产品包的快速部署；如果用户无需在 SAC 中部署 MySQL 实例或 PostgreSQL 实例，输入回车即可退出部署

    ```lang-text
    Whether copy run package for SAC use [y/N]:y
    ```

7. 提示配置 SequoiaDB 产品包所在目录，输入回车，选择默认目录 `/opt`；输入目录后按回车表示选择自定义目录

    ```lang-text
    Please enter SequoiaDB run package directory for SAC used [/opt]:
    ```

8. 提示配置 Sequoiasql-mysql 产品包所在目录，输入回车，选择默认目录 `/opt`；输入目录后按回车表示选择自定义目录

    ```lang-text
    Please enter Sequoiasql-mysql run package directory for SAC used [/opt]:
    ```

9. 提示配置 Sequoiasql-postgresql 产品包所在目录，输入回车，选择默认目录 `/opt`；输入目录后按回车表示选择自定义目录

    ```lang-text
    Please enter Sequoiasql-postgresql run package directory for SAC used [/opt]:
    ```

##登录SAC##

安装完成后，OM 会自动启动并开启 8000 端口的 web 服务，用户可以通过浏览器登陆 SAC，并进行集群的部署。假设安装 OM 的机器 IP 为 192.168.1.100，则在浏览器键入  `http://192.168.1.100:8000` 访问 SAC 服务。

输入登录用户名（默认为 admin）和密码（初始密码为 admin）后点击 **登录** 按钮

![登录SAC][login]

   > **Note:**  
   > 第一次使用 SAC 的用户，可以通过[一键部署][deployment_wizard]来完成集群安装。

##修改登录密码##

1. 在 SAC 页面右上方，点击用户名【admin】->【修改密码】
 
  ![修改SAC密码][reset_pwd_1]

2. 输入当前密码（初始密码为 admin）和新密码后点击 **确定** 按钮

    > **Note:**  
    > 用户需牢记新密码，SAC 暂不支持找回密码。

    ![登录SAC][reset_pwd_2]



[^_^]:
    本文使用的所有引用及链接
[download]:http://download.sequoiadb.com/cn/
[deployment_wizard]:manual/SAC/Deployment/deployment_wizard.md
[system_requirement]:manual/Deployment/env_requirement.md
[linux_suggest_setting]:manual/Deployment/linux_suggestion.md
[CreateOM]:manual/Manual/Sequoiadb_Command/Oma/createOM.md

[login]:images/SAC/login.png
[reset_pwd_1]:images/SAC/reset_pwd_1.png
[reset_pwd_2]:images/SAC/reset_pwd_2.png
[packet]:manual/SAC/Deployment/Deployment_Bystep/deploy_package.md