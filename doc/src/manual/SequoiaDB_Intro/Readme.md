[^_^]:
    SequoiaDB文档中心首页

> [SequoiaDB 3.6 版本说明][release_note]

[^_^]:
    一句话说明巨杉数据库的定位与本文的价值。
欢迎来到 [**SequoiaDB 巨杉数据库**][intro] 文档中心。SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库。

本文档中心旨在介绍 SequoiaDB 巨杉数据库的基本概念、数据库实例创建与管理方式、数据增删改查的基本语法、数据库集群管理的基本策略、以及性能调优和问题诊断的基本思路。

##快速入门##

在使用 SequoiaDB 巨杉数据库前，用户需要完成数据库存储引擎的安装，之后可以创建并使用关系型数据库实例，或直接通过 API 对分布式存储引擎进行操作。

[^_^]:
    展示重要内容的导航，让读者较容易上手掌握最重要的若干章节。
    TODO:需要增加 shell方法, shell操作符, 错误返回码 的链接

|安装部署|应用开发|运维监控|参考手册|
|----|----|----|----|
|[软硬件配置要求][install_requirement]|[MySQL 应用开发][mysql_development]|[监控][monitoring]|[Shell 方法][command]|
|[独立模式部署][quick_deployment]|[JSON 应用开发][json_development]|[扩缩容][expand]|[Shell 操作符][operator]|
|[集群模式部署][cluster_deployment]|[S3 开发][object_development]|[升级][upgrade]|[错误返回码][errorcode]|
|[容器模式部署][docker_deployment]|[Shell 命令行开发][shell_development]|[性能调优][performance]|[图形界面][sac]|

##产品下载##

用户可以在官网注册登陆并下载：
- [Docker 镜像/虚拟机镜像][docker]
- [SequoiaDB 安装包][download]
- [SequoiaDB 驱动及社区工具][tool]

同时，用户可以前往 [github][github] 下载 SequoiaDB 巨杉数据库源代码。

##技术支持##

+ 社区服务热线400-8038-339，或微信添加社区管理员 **sequoiadb111** 和 **sequoiadb333** 加入巨杉技术社区
+ 企业服务热线400-8038-339，或直接发送邮件至 sales_support@sequoiadb.com

##问题提交##

社区用户可以直接通过 [github][github] 提交在使用过程中所遇到的问题或修复代码。

##学习巨杉数据库##

+ [技术博客][blog]
+ [巨杉大学][university]
+ [文档中心][infocenter]
+ [巨杉问答][idea]
+ 微信添加社区管理员 **sequoiadb111** 和 **sequoiadb333** 加入巨杉技术社区

##授权##

SequoiaDB 巨杉数据库社区版使用 [AGPL-3.0][licensing] 协议授权。


[^_^]:
    本文使用到的所有链接及引用。
[intro]:manual/SequoiaDB_Intro/sequoiadb_intro.md
[install_requirement]:manual/Deployment/env_requirement.md
[quick_deployment]:manual/Deployment/standalone_deployment.md
[cluster_deployment]:manual/Deployment/cluster_deployment.md
[docker_deployment]:manual/Quick_Start/docker_deployment.md
[mysql_development]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Readme.md
[json_development]:manual/Database_Instance/Json_Instance/Development/JavaScript.md
[object_development]:manual/Database_Instance/Object_Instance/Readme.md
[shell_development]:manual/Manual/Sequoiadb_Command/location.md
[monitoring]:manual/Distributed_Engine/Maintainance/Monitoring/Readme.md
[expand]:manual/Distributed_Engine/Maintainance/Expand/Readme.md
[upgrade]:manual/Distributed_Engine/Maintainance/Upgrade/Readme.md
[performance]:manual/Distributed_Engine/Maintainance/Performance/Readme.md
[command]:manual/Manual/Sequoiadb_Command/Readme.md
[operator]:manual/Manual/Operator/Match_Operator/Readme.md
[errorcode]:manual/Manual/Sequoiadb_error_code.md

[^_^]:
    TODO:change to tools
[sac]:manual/SAC/Readme.md
[github]:https://github.com/sequoiadb/sequoiadb
[blog]:http://blog.sequoiadb.com/
[university]:http://www.sequoiadb.com/cn/university
[idea]:http://idea.sequoiadb.com/cn/
[infocenter]:http://doc.sequoiadb.com
[licensing]:https://github.com/SequoiaDB/SequoiaDB/blob/master/GNU-AGPL-3.0.txt
[docker]:http://download.sequoiadb.com/cn/vm
[tool]:http://download.sequoiadb.com/cn/driver
[download]:http://download.sequoiadb.com/cn/sequoiadb
[release_note]:manual/release_note.md
