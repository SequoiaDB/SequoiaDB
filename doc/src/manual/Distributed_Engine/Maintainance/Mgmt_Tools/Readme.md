[^_^]:
    数据管理工具
    作者：吴艳
    时间：20190430
    评审意见
    王涛：
    许建辉：
    市场部：20190522


SequoiaDB 巨杉数据库提供多种类型数据管理工具，包括数据日志文件 dump、数据文件检测、节点间数据一致性检测、数据库信息收集、大对象数据管理、密码管理、数据日志重放、容灾切换合并等，方便用户在各类应用场景下进行数据管理和维护。

本章详细介绍各类管理工具的基本概念、参数说明和使用方法等，指导用户对数据库的各种问题进行优化分析，使用不同类型的工具实现有效的数据管理和维护。

数据管理工具如下：
+ [快速部署工具][quickdeploy]：用命令行的方式快速部署 SequoiaDB/SequoiaSQL-MySQL/SequoiaSQL-PostgreSQL 
+ [命令行工具][sdb]：SequoiaDB 数据库的接口工具
+ [本地集群检查工具][sdblist]：sdblist 是 SequoiaDB 的本地集群检查工具
+ [数据导入工具][sdbimprt]：sdbimprt 是 SequoiaDB 的数据导入工具
+ [数据导出工具][sdbexprt]：sdbexprt 是 SequoiaDB 的数据导出工具
+ [数据库检测工具][dmsdump]：使用数据库检测工具检查数据库文件结构的正确性并给出结果报告
+ [数据库日志 dump 工具][dpsdump]：使用数据库日志 dump 工具解析同步日志文件的内容并给出结果报告
+ [节点数据一致性检测工具][consistency_check]：检查节点间数据是否完全一致
+ [数据库信息收集工具][info_collection]：收集 SequoiaDB 相关信息，如数据库配置信息、数据库日志信息、数据库所在主机的硬件信息和数据库、操作系统信息以及数据库快照信息等
+ [大对象工具][lobtools]：使用大对象工具导出、导入和迁移大对象数据
+ [密码管理工具][passwordtools]：使用密码管理工具管理数据库密码
+ [日志重放工具][log_replay]：使用日志重放工具在其它集群或节点重放归档日志
+ [容灾切换合并工具][split_merge]：当两个子网间出现网络分离无法访问时，使用工具进行集群分离；两个子网间网络连通后，使用工具进行集群合并
+ [索引升级工具][upgrade_index]：sdbupgradeidx 是 SequoiaDB 的索引升级工具


[^_^]:
    本文使用到的所有链接及引用。
[quickdeploy]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/quickdeploy.md
[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[dmsdump]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/dmsdump.md
[dpsdump]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/dpsdump.md
[consistency_check]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/consistency_check.md
[info_collection]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/info_collection.md
[lobtools]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/lobtools.md
[passwordtools]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md
[log_replay]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/log_replay.md
[split_merge]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md
[sdb]: manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdb.md
[sdblist]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdblist.md
[upgrade_index]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/upgrade_index.md