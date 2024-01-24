[^_^]: 

    监控
    作者：何嘉文
    时间：20190524
    评审意见

    王涛：
    许建辉：
    市场部：


数据库监控用于维护数据库性能和评估系统运行状况。用户可以通过收集数据库信息对系统运行状况进行监控，并对问题进行分析。例如：
- 根据业务场景评估硬件要求
- 分析 SQL 查询的性能
- 分析集合空间、集合和索引的使用情况
- 找出系统性能不佳的原因
- 评估优化操作的影响（例如：更改数据库配置参数、添加索引和修改 SQL 查询）


通过本章文档，用户可以了解 SequoiaDB 巨杉数据库的监控命令和监控工具。分析监控信息，是提高系统性能的重要手段。主要内容如下：

* [整体监控框架概述][architecture_url]
* [监控节点状态][monitor_node]
* [监控集群状态][monitor_cluster]
* [Sdbtop 工具][sdbtop_url]


[^_^]:
    本文使用到的所有链接及引用。
    
[architecture_url]: manual/Distributed_Engine/Maintainance/Monitoring/architecture.md
[sdbtop_url]: manual/Distributed_Engine/Maintainance/Monitoring/sdbtop.md
[monitor_cluster]:manual/Distributed_Engine/Maintainance/Monitoring/monitor_cluster.md
[monitor_node]:manual/Distributed_Engine/Maintainance/Monitoring/monitor_node.md