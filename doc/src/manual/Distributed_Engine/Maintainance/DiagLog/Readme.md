数据库日志记录是高可用数据库解决方案设计的重要部分。当用户需要从故障中恢复数据库，或同步主数据库和备数据库时，都需要使用数据库日志。

SequoiaDB 巨杉数据库的相关日志保存了有关数据库更改的记录，本章将向用户介绍以下三种日志及集群日志分析方法：

- [集群管理诊断日志][cm_log]：记录数据库集群被执行的所有操作
- [节点诊断日志][diaglog]：记录数据库节点执行过的操作信息
- [审计日志][auditlog]：记录用户对数据库执行的所有操作
- [集群日志分析方法][analyze_log]：sdbdiag.log 日志能够帮助用户快速定位具体失败原因，并且及时解决


[^_^]:
     本文使用的所有引用和链接
[diaglog]:manual/Distributed_Engine/Maintainance/DiagLog/diaglog.md
[cm_log]:manual/Distributed_Engine/Maintainance/DiagLog/cm_log.md
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md
[analyze_log]:manual/Distributed_Engine/Maintainance/DiagLog/analyze_log.md

