[^_^]:
    数据库安装

本文档主要介绍通过命令行方式在本地主机安装 SequoiaDB 巨杉数据库。如果有多台主机，每台主机都需要重复下述安装步骤。

##下载安装包##

用户可前往 SequoiaDB 巨杉数据库官网下载中心下载相应版本的[安装包][download_sequoiadb]。

##安装##

###安装说明###

- 安装过程需要使用 root 用户权限。
- 安装过程如果输入有误，可按 ctrl+退格键进行删除。
- 安装前需要参照[操作系统配置][os_setup]和 [Linux 环境推荐配置][linux_suggestion]调整 Linux 系统配置。

###安装步骤###

下述安装过程将以产品包 `sequoiadb-{version}-linux_x86_64-installer.run` 为例，介绍具体安装步骤。

1. 以 root 用户登陆目标主机，解压 SequoiaDB 巨杉数据库产品包，并为解压得到的 `sequoiadb-{version}-linux_x86_64-installer.run` 安装包赋可执行权限

    ```lang-bash
    # tar -zxvf sequoiadb-{version}-linux_x86_64-installer.tar.gz
    # chmod u+x sequoiadb-{version}-linux_x86_64-installer.run
    ```

2. 运行 `sequoiadb-{version}-linux_x86_64-installer.run` 包

    ```lang-bash
    # ./sequoiadb-{version}-linux_x86_64-installer.run --mode text
    ```

3. 提示选择向导语言，输入 2，选择中文

    ```lang-text
    Language Selection
    
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] : 2
    ```

4. 显示安装协议，输入回车，忽略阅读并同意协议；输入 2 后按回车表示读取全部文件

    ```lang-text
    ----------------------------------------------------------------------------
    欢迎使用 SequoiaDB 安装向导。

    ----------------------------------------------------------------------------
    重要信息：请仔细阅读

    下面提供了两个许可协议。

    1. SequoiaDB 评估程序的最终用户许可协议
    2. SequoiaDB 最终用户许可协议

    如果被许可方为了生产性使用目的（而不是为了评估、测试、试用“先试后买”或演示）获得本程序，单击下面的“接受”按钮即表示被许可方接受 SequoiaDB 最终用户许可协议，且不作任何修改。

    如果被许可方为了评估、测试、试用“先试后买”或演示（统称为“评估”）目的获得本程序：单击下面的“接受”按钮即表示被许可方同时接受（i）SequoiaDB 评估程序的最终用户许可协议（“评估许可”），且不作任何修改；和（ii）SequoiaDB 最终用户程序许可协议（SELA），且不作任何修改。

    在被许可方的评估期间将适用“评估许可”。

    如果被许可方通过签署采购协议在评估之后选择保留本程序（或者获得附加的本程序副本供评估之后使用），SequoiaDB 评估程序的最终用户许可协议将自动适用。

    “评估许可”和 SequoiaDB 最终用户许可协议不能同时有效；两者之间不能互相修改，并且彼此独立。

    这两个许可协议中每个协议的完整文本如下。

    评估程序的最终用户许可协议



    [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
    [2] 查看详细的协议内容
    请选择选项 [1] : 
    ```

5. 提示指定安装路径，输入回车，选择默认路径 `/opt/sequoiadb`；输入路径后按回车表示选择自定义路径

    ```lang-text
    ----------------------------------------------------------------------------
    请选择存在的版本或者进行新的安装
    
    请选择存在的版本或者进行新的安装 [/opt/sequoiadb]: 
    ```

6. 提示配置 Linux 用户名和用户组，该用户名用于运行 SequoiaDB 服务，输入回车，选择创建默认的用户名（sdbadmin）和用户组（sdbadmin_group）；输入用户名和用户组后按回车表示选择自定义的用户名和用户组

    ```lang-text
    ----------------------------------------------------------------------------
    数据库管理用户配置
    
    用户名 [sdbadmin]: 
    
    ----------------------------------------------------------------------------
    用户组 [sdbadmin_group]: 
    ```

7. 提示配置刚才创建的 Linux 用户密码，输入回车，选择使用默认密码（Admin@1024）；输入密码后按回车表示选择自定义密码

    ```lang-text
    ----------------------------------------------------------------------------
    密码 [********] :
    确认密码 [********] :
    ```

8. 提示是否强制安装，输入回车，选择不强制安装；输入 y 后按回车表示选择强制安装

    ```lang-text
    ----------------------------------------------------------------------------
    是否强制安装？强制安装时可能会强杀残留进程
    
    是否强制安装 [y/N]: 
    ```

9. 提示选择允许 SequoiaDB 相关进程开机自启动，输入回车，选择开机自启动

    ```lang-text
    ----------------------------------------------------------------------------
    是否允许Sequoiadb相关进程开机自启动？
    
    Sequoiadb相关进程开机自启动 [Y/n]: 
    ```

10. 提示配置服务端口，输入回车，选择使用默认的服务端口号（11790）；输入端口号后按回车表示选择自定义端口

    ```lang-text
    ----------------------------------------------------------------------------
    配置SequoiaDB集群管理服务端口，集群管理用于远程启动添加和启停数据库节点
    
    集群管理服务端口 [11790]: 
    ```

    > **Note:**
    >
    > 用户需要在多台主机上安装部署 SequoiaDB 时，所有主机配置的服务端口必须设置为相同的值。

11. 输入回车，确认继续

    ```lang-text
    ----------------------------------------------------------------------------
    安装程序已经准备好将 SequoiaDB 安装到您的电脑。
    
    您确定要继续吗？ [Y/n]: 
    ```

12. 安装完成

    ```lang-text
    ----------------------------------------------------------------------------
    正在安装 SequoiaDB Server 至您的电脑中，请稍候。
    
    正在安装
    0% ______________ 50% ______________ 100%
    #########################################
    
    ----------------------------------------------------------------------------
    安装程序已经将 SequoiaDB Server 安装于您的电脑中。
    
    安装成功，安装报告可查看：/opt/sequoiadb/install_report
    ```

当所有的主机都安装了 SequoiaDB 后，用户可参考[规划数据库部署][readme]并结合自身情况，选择部署单机模式或者集群模式的环境。只有成功部署了环境，才能使用 SequoiaDB 进行数据操作。

[^_^]:
     本文使用的所有引用及链接
[download_sequoiadb]:http://download.sequoiadb.com/cn/index-cat_id-1 
[install_requirement]:manual/Deployment/env_requirement.md
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[catalog_node]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[coord_node]:manual/Distributed_Engine/Architecture/Node/coord_node.md
[os_setup]:manual/Deployment/env_requirement.md
[linux_suggestion]:manual/Deployment/linux_suggestion.md
[sac]:manual/SAC/install_login.md
[readme]:manual/Deployment/Readme.md#规划数据库部署
