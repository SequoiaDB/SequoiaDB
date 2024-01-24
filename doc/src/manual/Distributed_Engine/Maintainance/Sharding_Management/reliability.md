[^_^]:
    分区可靠性管理
    作者：魏彰凯
    时间：20190531
    评审意见
    王涛：
    许建辉：
    市场部：20190820
       

在 SequoiaDB 巨杉数据库中，分区可靠性管理主要指数据的可靠性管理。SequoiaDB 在复制组内采用多数据副本，建立完备的高可用数据库集群以保障数据的可靠性。本文从多副本选择、[主备一致性][primary_secondary_consistency]、[高可用][high_avaliability]和[容灾][disaster_recovery]等方面来说明分区可靠性管理。

多副本的选择
----

SequoiaDB 使用改进过的 Raft 选举协议进行[复制组选举][repl_election]，确定哪个成员是主节点，副本集推荐奇数个成员，以确保选举的顺利进行。在一个复制组内，节点的个数上限为 7。根据不同的高可用容灾需要，复制组内节点数建议值为：3、5、7。
推荐用户创建三副本的复制组，三个副本能够提供足够的冗余承受系统故障。如果有特定的容灾需求，可以参考容灾部署方案：[同城双中心][twodatacenter]、[同城三中心][threedatacenter]、[两地三中心][twocity_threedatacenter]和[三地五中心][threecity_fivedatacenter]。

主备一致性
---

SequoiaDB 中的一致性策略包含强一致性、最终一致性和写大多数。这三种一致性策略分别适配不同的应用场景，下面列举不同场景下一致性策略的选择方式：

- 联机交易型业务：为保证数据安全性，并且可以牺牲写入性能时，推荐使用强一致性策略
- 数据中台型业务：在一定时间内，查询数据可以不是最新数据。批量插入数据时，响应速度快，推荐使用最终一致性策略
- 容灾部署模式：异地机房间的网络带宽有限，往往不能支撑高效的数据实时同步，推荐使用写大多数策略，配置为主机房或者同城机房的副本写入成功后，返回写入成功

高可用
----

高可用指通过 SequoiaDB 复制组内多副本机制的集群架构，缩短因日常维护操作和突发系统崩溃所导致的停机时间。在 SequoiaDB 复制组内的多副本机制下，每个副本都可以接管主节点任务，不再局限于传统数据库的单点故障问题。

- 复制组内，小于半数的从节点故障时，不影响数据库的使用，业务系统无感知
- 复制组内，主节点故障时，从节点会自动发起选举，选出新的主节点，保证数据库的正常使用
- 复制组内，半数以上节点故障时，主节点会自动降级为从节点，提供只读服务

容灾
----

容灾是指建立多地的容灾中心，该中心是主数据中心的一个可用复制，在灾难发生之后确保原有的数据不会丢失或遭到破坏。容灾中心数据可以是主中心生产数据的完全实时复制，也可以比主中心数据稍微落后，但一定是可用的。采用的主要技术是数据复制和数据备份。

### 节点健康检测

SequoiaDB 提供了完善的高可用和容灾部署架构，部分数据节点的故障不被数据库运维人员感知，需要运维人员主动检查数据节点状态，发现问题并解决节点故障。SequoiaDB 提供的[节点健康检测快照][health]可以查看数据库节点的健康状态。

```lang-javascript
> db.snapshot( SDB_SNAP_HEALTH )
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver1:11800",
  "IsPrimary": true,
  "ServiceStatus": true,
  "Status": "Normal",
  "BeginLSN": {
    "Offset": 76776,
    "Version": 3
  },
  "CurrentLSN": {
    "Offset": 92356,
    "Version": 4
  },
  "CommittedLSN": {
    "Offset": 92356,
    "Version": 4
  },
  "CompleteLSN": 92428,
  "LSNQueSize": 0,
  "NodeID": [
    1,
    1
  ],
  "DataStatus": "Normal",
  "SyncControl": false,
  "Ulimit": {
    "CoreFileSize": 0,
    "VirtualMemory": -1,
    "OpenFiles": 60000,
    "NumProc": 22521,
    "FileSize": -1
  },
  "ResetTimestamp": "2019-06-04-23.54.06.410178",
  "ErrNum": {
    "SDB_OOM": 0,
    "SDB_NOSPC": 0,
    "SDB_TOO_MANY_OPEN_FD": 0
  },
  "Memory": {
    "LoadPercent": 3,
    "TotalRAM": 2972622848,
    "RssSize": 95174656,
    "LoadPercentVM": 0,
    "VMLimit": -1,
    "VMSize": 2542215168
  },
  "Disk": {
    "Name": "/dev/mapper/centos_test-root",
    "LoadPercent": 85,
    "TotalSpace": 29521608704,
    "FreeSpace": 4197621760
  },
  "FileDesp": {
    "LoadPercent": 0,
    "TotalNum": 60000,
    "FreeNum": 59938
  },
  "StartHistory": [
    "2019-06-04-23.54.06.650502",
    "2019-06-03-23.44.24.856599",
    "2019-05-30-09.45.40.948906",
    "2019-05-29-21.28.11.460570",
    "2019-05-29-10.22.33.248294"
  ],
  "AbnormalHistory": [
    "2019-05-30-09.45.40.948906"
  ],
  "DiffLSNWithPrimary": 0
}
...
```

[^_^]:
    本文使用到的所有链接

[repl_election]:manual/Distributed_Engine/Architecture/Replication/election.md
[primary_secondary_consistency]:manual/Distributed_Engine/Architecture/Replication/primary_secondary_consistency.md
[high_avaliability]:manual/Distributed_Engine/Maintainance/HA_DR/high_avaliability.md
[disaster_recovery]:manual/Distributed_Engine/Maintainance/HA_DR/disaster_recovery.md
[twodatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/twodatacenter.md
[threedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/threedatacenter.md
[twocity_threedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/twocity_threedatacenter.md
[threecity_fivedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/threecity_fivedatacenter.md
[health]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md

