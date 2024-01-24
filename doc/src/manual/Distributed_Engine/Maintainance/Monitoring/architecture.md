[^_^]: 

    整体监控框架概述
    作者：何嘉文
    时间：20190524
    评审意见

    王涛：
    许建辉：
    市场部：20190820


监控命令
----

SequoiaDB 巨杉数据库可以通过快照和列表监控数据库系统。

- 快照是获取系统当前详细状态的命令，具体快照类型可以查看[数据库快照][snapshot]。

- 列表是获取系统当前轻量级状态的命令，具体的列表类型可以查看[数据库列表][list]。

监控工具
----

sdbtop 是 SequoiaDB 的性能监控工具。通过 sdbtop，用户可以监控和查看集群中各个节点的监视信息。sdbtop 操作说明可参考[数据库性能工具][sdbtop_url]。

图形化界面
----

SAC 是一个图形化界面管理系统，可以监控数据库系统和主机性能。SAC 监控可参考 SAC 的[监控][sac_monitor_url]章节。

[^_^]:
    本文使用到的所有链接及引用
[snapshot]: manual/Manual/Snapshot/Readme.md
[list]: manual/Manual/List/Readme.md
[sdbtop_url]: manual/Distributed_Engine/Maintainance/Monitoring/sdbtop.md

[sac_monitor_url]: manual/SAC/Monitor/Readme.md