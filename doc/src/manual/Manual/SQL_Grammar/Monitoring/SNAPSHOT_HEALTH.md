
节点健康检测快照可以列出数据库中所有节点的健康信息。

##标识##

$SNAPSHOT_HEALTH

##字段信息##

| 字段名               | 类型      | 描述                                                            |
| -------------------- | --------- | --------------------------------------------------------------- |
| NodeName             | string    | 节点名，格式为<主机名>:<服务名>                                 |
| IsPrimary            | boolean   | 是否主节点                                                      |
| ServiceStatus        | boolean   | 是否为可提供服务状态 <br>一些特殊状态，例如[全量同步][architecture]时，服务状态为 false |              
| Status               | string    |  节点状态，取值如下：<br/> "Normal"：正常工作状态 <br/> "Shutdown"：正在关闭状态，表示节点正在被关闭 <br/> "Rebuilding"：重新构建状态，如节点异常重启后，无法与其他节点进行数据同步，则节点会进入该状态，重新构建数据 <br/> "FullSync"：全量同步状态 <br/> "OfflineBackup"：[数据备份][regular_bar]状态 |
| BeginLSN.Offset      | int64   | 起始 LSN 的偏移 |
| BeginLSN.Version     | int32   | 起始 LSN 的版本号 |
| CurrentLSN.Offset    | int64   | 当前 LSN 的偏移 |
| CurrentLSN.Version   | int32   | 当前 LSN 的版本号 |
| CommittedLSN.Offset  | int64   | 已提交 LSN 的偏移 |
| CommittedLSN.Version | int32   | 已提交 LSN 的版本号 |
| CompleteLSN          | int64     | 已完成 LSN 的偏移                                               |
| LSNQueSize           | int32     | 等待同步的 LSN 队列长度                                         |
| NodeID               | array  | 节点的 ID 信息，格式为[<分区组 ID>, <节点 ID>] <br> 在 standalone 模式下，该字段为[0, 0]                 |
| DataStatus           | string    | 数据状态，取值如下：<br/> "Normal"：正常状态 <br/> "Repairing"：修复状态，当节点状态为"Rebuilding"或"FullSync"时，数据状态为"Repairing"  <br/> "Fault"：错误状态，当节点异常启动，且节点状态不为"Rebuilding"或"FullSync"时，数据状态为"Fault"  |
| SyncControl          | boolean   | 节点是否处于同步控制                                            |
| Ulimit.CoreFileSize  | int64  | 节点进程的 core 文件大小限制，单位为字节，-1 表示 unlimited |
| Ulimit.VirtualMemory | int64  | 节点进程的虚拟内存限制，单位为字节，-1 表示 unlimited |
| Ulimit.OpenFiles     | int64  | 节点进程的文件句柄数限制                         |
| Ulimit.NumProc       | int64  | 节点进程的线程数限制，-1 表示 unlimited          |
| Ulimit.FileSize      | int64  | 节点进程的文件大小限制，单位为字节，-1 表示 unlimited      |
| Ulimit.StackSize     | int64  | 节点进程的栈空间大小限制，单位为字节，-1 表示 unlimited |
| ResetTimestamp       | timestamp | 重置快照的时间                                                  |
| ErrNum.SDB_OOM              | int64  | 节点发生 SDB_OOM 错误的次数                   |
| ErrNum.SDB_NOSPC            | int64  | 节点发生 SDB_NOSPC 错误的次数                 |
| ErrNum.SDB_TOO_MANY_OPEN_FD | int64  | 节点发生 SDB_TOO_MANY_OPEN_FD 错误的次数      |
|  Memory.LoadPercent   | int32  | 节点进程占用 RAM 的百分比                  |
| Memory.TotalRAM      | int64  | 节点所在操作系统的总 RAM 大小，单位为字节  |
| Memory.RssSize       | int64  | 节点进程占用的 RAM 大小，单位为字节        |
| Memory.LoadPercentVM | int32  | 节点进程占用虚拟空间的百分比               |
| Memory.VMLimit       | int64  | 节点进程虚拟空间限制，单位为字节           |
| Memory.VMSize        | int64  | 节点进程占用的虚拟空间，单位为字节         |
| Disk.Name        | string | 节点路径所在的磁盘名称                      |
| Disk.LoadPercent | int32  | 节点路径占用磁盘的百分比                    |
| Disk.TotalSpace  | int64  | 节点路径所在的磁盘空间大小，单位为字节      |
| Disk.FreeSpace   | int64  | 节点路径所在的磁盘剩余空间大小，单位为字节  |
| FileDesp.LoadPercent | int32  | 节点进程占用的文件句柄的百分比 |
| FileDesp.TotalNum    | int64  | 节点进程文件句柄限制           |
| FileDesp.FreeNum     | int64  | 节点进程剩余的文件句柄个数     |
| FTStatus | string | 容错状态，取值如下：<br>"NOSPC"：磁盘空间不足<br>"DEADSYNC"：节点数据不同步 <br>"SLOWNODE"：节点数据同步过慢<br>"TRANSERR"：节点事务异常 |
| StartHistory         | array     | 节点启动历史（只取最新的十条记录）                              |
| AbnormalHistory      | array     | 节点异常后启动历史（只取最新的十条记录）                        |
| DiffLSNWithPrimary   | int64     | 与主节点的 LSN 差异                                             |

> **Note：**  
>
> - 协调节点的快照返回所有节点的信息，非协调节点返回自身节点的信息。  
> - 备节点在计算与主节点的 LSN 差异时，所取的主节点 LSN 可能是两秒钟前的，因此 DiffLSNWithPrimary 可能与实际值存在一定偏差。（两秒是一个心跳间隔）


##示例##

查看数据节点 20000 上的健康检测信息

```lang-javascript
> db.exec("select * from $SNAPSHOT_HEALTH")
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:20000",
  "IsPrimary": true,
  "ServiceStatus": true,
  "Status": "Normal",
  "FTStatus": "",
  "BeginLSN": {
    "Offset": 0,
    "Version": 1
  },
  "CurrentLSN": {
    "Offset": 1050290908,
    "Version": 1
  },
  "CommittedLSN": {
    "Offset": 1050290908,
    "Version": 1
  },
  "CompleteLSN": 1050290984,
  "LSNQueSize": 0,
  "NodeID": [
    1000,
    1000
  ],
  "DataStatus": "Normal",
  "SyncControl": false,
  "Ulimit": {
    "CoreFileSize": 0,
    "VirtualMemory": -1,
    "OpenFiles": 1024,
    "NumProc": 23711,
    "FileSize": -1
  },
  "ResetTimestamp": "2019-05-31-09.37.59.316262",
  "ErrNum": {
    "SDB_OOM": 0,
    "SDB_NOSPC": 0,
    "SDB_TOO_MANY_OPEN_FD": 0
  },
  "Memory": {
    "LoadPercent": 11,
    "TotalRAM": 6257471488,
    "RssSize": 701349888,
    "LoadPercentVM": 0,
    "VMLimit": -1,
    "VMSize": 2491211776
  },
  "Disk": {
    "Name": "/dev/mapper/vgdata-lvdata1",
    "LoadPercent": 34,
    "TotalSpace": 211139878912,
    "FreeSpace": 138432405504
  },
  "FileDesp": {
    "LoadPercent": 4,
    "TotalNum": 1024,
    "FreeNum": 973
  },
  "StartHistory": [
    "2019-05-31-09.37.59.581769"
  ],
  "AbnormalHistory": [],
  "DiffLSNWithPrimary": 0
}
```



[^_^]:
    本文使用的所有引用及链接
[architecture]:manual/Distributed_Engine/Architecture/Replication/architecture.md#全量同步
[regular_bar]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md
