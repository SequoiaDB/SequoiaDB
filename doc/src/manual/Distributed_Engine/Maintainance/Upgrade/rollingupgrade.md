[^_^]:
    滚动升级
    作者：杨上德
    时间：20190818
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190911


滚动升级是一种在线升级方式，相比离线升级，滚动升级可保证在部分或全部服务可用的情况下完成软件的升级。对于采用分布式架构的 SequoiaDB 巨杉数据库，在集群规模大且支撑业务多且复杂时，尽量减少业务中断的滚动升级具有重要的意义。

由于 SequoiaDB 的升级通常会涉及到多台主机以及多种类型的节点，滚动升级需要按照指定的流程执行。升级流程需注意以下几点：

- 尽量选择在业务量最小的时间段内进行升级
- 对于包含多个节点的复制组（数据节点组和编目节点组），采用主备节点滚动升级策略，先对备节点进行升级，最后对主节点进行升级
- 对于协调节点，如果前端业务系统配置了负载均衡则影响不大；如果没有配置负载均衡，建议先将业务连接的协调节点调整到其它主机上的协调节点，以减少升级过程对业务的影响

具体的升级流程如下：

1. 选择一台拥有复制组主节点最少的主机，查看有哪些复制组的主节点在该主机上
2. 使用 [reelect()][reelect] 命令，将这些复制组的主节点切换到其它主机上
3. 使用 [startMaintenanceMode()][startMaintenanceMode] 命令，对需要升级的节点开启运维模式
4. 按照离线升级中的[升级步骤][offlineupgrade]前 5 步完成本主机上的软件升级，在正常完成升级的情况下，所有的节点应已正常启动并重新加入到集群中
5. 使用 [SDB_LIST_GROUPMODES][SDB_LIST_GROUPMODES] 查看复制组当前的模式，如升级成功的节点仍处于运维模式，请手动调用 [stopMaintenanceMode()][stopMaintenanceMode] 命令从运维模式中移除这些节点，否则这些节点不会参与复制组选主
6. 选取下一个节点按照上述步骤完成升级，直至完成所有主机上的软件升级
7. 按照离线升级中的[升级步骤][offlineupgrade]第 6 步完成索引升级

> **Note:**
>
> 在用户将 SequoiaDB 由低版本滚动升级至 v3.6/5.0.3 及以上版本的过程中，禁止执行 [createIndex][createIndex]、[dropIndex][dropIndex]、[copyIndex][copyIndex]、[split][split] 等涉及索引和分区键的操作。


[^_^]:
    本文中用到的所有链接

[compatible_list]:manual/Distributed_Engine/Maintainance/Upgrade/compatibility.md
[reelect]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md
[offlineupgrade]:manual/Distributed_Engine/Maintainance/Upgrade/offline.md#升级
[createIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md
[copyIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/copyIndex.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md
[SDB_LIST_GROUPMODES]:manual/Manual/List/SDB_LIST_GROUPMODES.md
[startMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startMaintenanceMode.md
[stopMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stopMaintenanceMode.md
