[^_^]:
    复制组监控
    作者：余婷
    时间：20190426
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190531

用户可以通过 SDB Shell 或者其他驱动监控复制组，查看复制组节点运行状态。

节点健康检测快照
----

SequoiaDB 巨杉数据库提供多个快照，以查看当前数据库系统的各种状态。其中节点健康检测快照 [SDB_SNAP_HEALTH][health] 可以查看到各个节点的健康状态。用户可以从节点运行状态、复制组同步状态、系统资源使用情况、节点启动历史等不同的方面，来评估复制组节点的健康状况。

连接协调节点，查看复制组 group1 的节点健康状态

```lang-bash
> var db = new Sdb( 'sdbserver1', 11810 )
> db.snapshot( SDB_SNAP_HEALTH, { GroupName: "group1" }  )
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver1:11820",
  "IsPrimary": true,
  "ServiceStatus": true,
  "Status": "Normal",
  "BeginLSN": {
    "Offset": 0,
    "Version": 1
  },
  "CurrentLSN": {
    "Offset": 9610788,
    "Version": 1
  },
  "CommittedLSN": {
    "Offset": 76,
    "Version": 1
  },
  "CompleteLSN": 9610868,
  "LSNQueSize": 0,
  "NodeID": [
    1000,
    1000
  ],
  "DataStatus": "Normal",
  "SyncControl": false,
  "Ulimit": {
    "CoreFileSize": -1,
    "VirtualMemory": -1,
    "OpenFiles": 1024,
    "NumProc": 23948,
    "FileSize": -1
  },
  "ResetTimestamp": "2018-03-09-09.47.04.826497",
  "ErrNum": {
    "SDB_OOM": 1,
    "SDB_NOSPC": 0,
    "SDB_TOO_MANY_OPEN_FD": 1
  },
  "Memory": {
    "LoadPercent": 3,
    "TotalRAM": 3157524480,
    "RssSize": 96591872,
    "LoadPercentVM": 0,
    "VMLimit": -1,
    "VMSize": 2380341248
  },
  "Disk": {
    "Name": "/dev/mapper/vgdata-lvdata1",
    "LoadPercent": 69,
    "TotalSpace": 52836298752,
    "FreeSpace": 16025624576
  },
  "FileDesp": {
    "LoadPercent": 3,
    "TotalNum": 1024,
    "FreeNum": 985
  },
  "StartHistory": [
    "2018-01-24-15.55.58.374162",
    "2018-01-24-15.55.00.318481"
  ],
  "CrashHistory": [
    "2018-01-24-15.55.58.374162"
  ],
  "DiffLSNWithPrimary": 0
}
```

### 节点基本信息 ###

+ NodeName：节点名由主机名和端口号组成
+ NodeID：第一个元素为复制组 ID，第二个元素为节点 ID

### 节点运行状态 ###

+ Status：该节点处于正常、正在 Rebuild、正在[全量同步][fullsync]或正在[数据备份][backup_offline]
+ ServiceStatus：该节点是否能对外提供读写服务
+ DataStatus：该节点数据是否损坏

> **Note:**
>
> Rebuild 状态：当节点启动时发现数据损坏，则进行数据重组以恢复数据

### 复制组同步状态 ###

+ IsPrimary：该节点是否为复制组的[主节点][primary]
+ BeginLSN：在[事务日志][replicalog]文件中记录的第一条 LSN
+ CurrentLSN：该节点当前处理的事务日志 LSN
+ CommittedLSN：该节点已刷盘的事务日志 LSN
+ CompleteLSN：[备节点][slave]已重放完成的事务日志 LSN
+ LSNQueSize：备节点待重放的事务日志数量
+ DiffLSNWithPrimary：该节点与主节点的 LSN 差异。备节点在计算与主节点的 LSN 差异时，所取的主节点 LSN 可能是一个[心跳][heartbeat]间隔前的，因此 DiffLSNWithPrimary 可能与实际值存在一定偏差。
+ SyncControl：该节点是否处于同步控制。默认情况下，当主备节点 LSN 差距过大时，为避免引发备节点的全量同步，主节点会主动降低处理操作的速度。用户可以通过配置 [syncstrategy][syncstrategy] 参数来修改同步控制的策略。

> **Note:**
>
> 每条 LSN 由 Offset 和 Version 两个字段组成：
> * Offset 是指该条事务日志在日志文件中的偏移；
> * 复制组每次切换主节点，Version 都会递增 1。

### 系统资源使用情况 ###

+ Ulimit：该节点进程的 ulimit 限制
+ Memory：该节点物理内存和虚拟内存的使用情况
+ Disk：该节点数据目录所在的磁盘的使用情况
+ FileDesp：该节点文件句柄使用情况
+ ErrNum：该节点发生内存不足、磁盘空间不足、文件句柄不足等错误的次数

### 节点启动历史 ###

+ StartHistory：该节点启动历史（只取最新的十条记录）
+ CrashHistory：该节点异常停止后的启动历史（只取最新的十条记录）

重置快照
----

用户可以通过 [resetSnapshot()][resetSnapshot] 重置节点健康检测快照中的某些字段。

1. 重置复制组 group1 的节点健康检测快照

   ```lang-bash
   > db.resetSnapshot( { Type: "health", GroupName: "db1" })
   ```

2. 查询字段 ErrNode、StartHistory、CrashHistory 是否已被置为空

   ```lang-bash
   > db.snapshot( SDB_SNAP_HEALTH, { GroupName: "group1" }, { NodeName: null, ErrNode: null, StartHistory: null,    AbnormalHistory: null }  )
   {
     "NodeName": "sdbserver1:11820",
     "ErrNum": {
       "SDB_OOM": 0,
       "SDB_NOSPC": 0,
       "SDB_TOO_MANY_OPEN_FD": 0
     },
     "StartHistory": [],
     "CrashHistory": []
   }
   ```

[^_^]:
    本文使用到的所有链接及引用。
[health]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[primary]:manual/Distributed_Engine/Architecture/Replication/architecture.md#主节点
[fullsync]:manual/Distributed_Engine/Architecture/Replication/architecture.md#全量同步
[backup_offline]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md
[replicalog]:manual/Distributed_Engine/Architecture/Replication/architecture.md#事务日志
[heartbeat]:manual/Distributed_Engine/Architecture/Replication/election.md#节点心跳
[slave]:manual/Distributed_Engine/Architecture/Replication/architecture.md#备节点
[syncstrategy]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[replicalog]:manual/Distributed_Engine/Architecture/Replication/architecture.md#事务日志
[replicalog]:manual/Distributed_Engine/Architecture/Replication/architecture.md#事务日志
[resetSnapshot]:manual/Manual/Sequoiadb_Command/Sdb/resetSnapshot.md
