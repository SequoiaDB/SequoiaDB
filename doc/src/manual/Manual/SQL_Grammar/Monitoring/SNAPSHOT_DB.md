
数据库快照可以列出当前数据库节点中主要的状态与性能监控参数。

通过协调节点查询快照，将返回所有节点的快照信息，每个数据节点或编目节点产生一条记录；通过数据节点或编目节点查询快照，将返回当前节点的快照信息。

##标识##

$SNAPSHOT_DB

##字段信息##

| 字段名                | 类型   | 描述                                                                            |
| --------------------- | ------ | ------------------------------------------------------------------------------- |
| NodeName              | string | 节点名，格式为< HostName >:< ServiceName >                                      |
| HostName              | string | 数据库节点所在物理节点的主机名                                                  |
| ServiceName           | string | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标识                  |
| GroupName             | string | 该逻辑节点所属的分区组名，standalone 模式下该字段为空字符串                     |
| IsPrimary             | boolean| 该节点是否为主节点，standalone 模式下该字段为 false                             |
| ServiceStatus         | boolean| 是否为可提供服务状态。<br>一些特殊状态，例如[全量同步][architecture]会使该状态为 false |
| Status                | string | 节点状态，取值如下：<br/>             "Normal"：正常工作状态<br/>             "Shutdown"：正在关闭状态，表示节点正在被关闭<br/>             "Rebuilding"：重新构建状态，如节点异常重启后，无法与其他节点进行数据同步时，节点会进入该状态，重新构建数据<br/>             "FullSync"：[全量同步][architecture]状态<br/>             "OfflineBackup"：[数据备份][regular_bar]状态  |
| FTStatus              | string | 容错状态，取值如下：<br> "NOSPC"：磁盘空间不足 <br>"DEADSYNC"：节点数据不同步 <br> "SLOWNODE"：节点数据同步过慢 <br> "TRANSERR"：节点事务异常 |
| BeginLSN.Offset       | int64  | 起始 LSN 的偏移                                                                 |
| BeginLSN.Version      | int32  | 起始 LSN 的版本号                                                               |
| CurrentLSN.Offset     | int64  | 当前 LSN 的偏移                                                                 |
| CurrentLSN.Version    | int32  | 当前 LSN 的版本号                                                               |
| CommittedLSN.Offset   | int64  | 已提交 LSN 的偏移                                                               |
| CommittedLSN.Version  | int32  | 已提交 LSN 的版本号                                                             |
| CompleteLSN           | int64  | 已完成 LSN 的偏移                                                               |
| LSNQueSize            | int32  | 等待同步的 LSN 队列长度                                                           |
| TransInfo.TotalCount  | int32  | 正在执行的事务数量                                                              |
| TransInfo.BeginLSN    | int64  | 正在执行的事务的起始 LSN 的偏移                                                 |
| NodeID                | array  | 节点的 ID，格式为[ <分区组 ID>, <节点 ID> ]<br>在 standalone 模式下，该字段为[ 0，0 ] |
| Version.Major         | int32  | 数据库主版本号                                                                  |
| Version.Minor         | int32  | 数据库子版本号                                                                  |
| Version.Fix           | int32  | 数据库修复版本号                                                                |
| Version.Release       | int32  | 数据库内部版本号                                                                |
| Version.GitVersion    | string | 数据库发行版本号                                                                |
| Version.Build         | string | 数据库编译时间                                                                  |
| Editon                | string | “Enterprise”表示企业版（社区版中无该字段）                                |
| CurrentActiveSessions | int32  | 当前活动会话                                                                |
| CurrentIdleSessions   | int32  | 当前非活动会话，一般来说非活动会话意味着 EDU 存在线程池中等待分配               |
| CurrentSystemSessions | int32  | 当前系统会话，为当前活动用户 EDU 数量 |
| CurrentTaskSessions   | int32  | 后台任务会话数量                                                                |
| CurrentContexts       | int32  | 当前上下文数量                                                                  |
| ReceivedEvents        | int32  | 当前分区接收到的事件请求总数                                                    |
| Role                  | string | 当前节点角色                                                                    |
| Disk.DatabasePath     | string | 数据库所在路径                                                                  |
| Disk.LoadPercent      | int32  | 数据库路径磁盘占用率百分比                                                      |
| Disk.TotalSpace       | int64  | 数据库路径总空间，单位为字节                                                  |
| Disk.FreeSpace        | int64  | 数据库路径空闲空间，单位为字节                                                |
| TotalNumConnects      | int32  | 数据库连接请求数量                                                              |
| TotalQuery            | int64  | 总查询数量（广义查询，泛指在数据库上执行的所有操作）                               |
| TotalSlowQuery        | int64  | 总慢查询数量（广义查询，泛指在数据库上执行的所有操作）                             |
| TotalTransCommit      | int64  | 总事务提交数量                                                                  |
| TotalTransRollback    | int64  | 总事务回滚数量                                                                  |
| TotalDataRead         | int64  | 总数据读请求                                                                    |
| TotalIndexRead        | int64  | 总索引读请求                                                                    |
| TotalDataWrite        | int64  | 总数据写请求                                                                    |
| TotalIndexWrite       | int64  | 总索引写请求                                                                    |
| TotalUpdate           | int64  | 总更新记录数量                                                                  |
| TotalDelete           | int64  | 总删除记录数量                                                                  |
| TotalInsert           | int64  | 总插入记录数量                                                                  |
| ReplUpdate            | int64  | 复制更新记录数量                                                                |
| ReplDelete            | int64  | 复制删除记录数量                                                                |
| ReplInsert            | int64  | 复制插入记录数量                                                                |
| TotalSelect           | int64  | 总选择记录数量                                                                  |
| TotalRead             | int64  | 总读取记录数量                                                                  |
| TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| TotalReadTime         | int64  | 总读取时间，单位为毫秒                                                        |
| TotalWriteTime        | int64  | 总写入时间，单位为毫秒                                                        |
| ActivateTimestamp     | timestamp | 数据库节点启动时间                                                         |
| ResetTimestamp        | timestamp | 重置快照的时间                               |
| UserCPU               | double | 用户 CPU，单位为秒                                                            |
| SysCPU                | double | 系统 CPU，单位为秒                                                            |
| freeLogSpace          | int64  | 空闲日志空间，单位为字节                                                      |
| vsize                 | int64  | 虚拟内存使用量，单位为字节                                                    |
| rss                   | int64  | 物理内存使用量，单位为字节                                                    |
| fault                 | int64  | 每秒访问失败数（仅支持 Linux），数据被交换出物理内存，放到 swap                 |
| TotalMapped           | int64  | mmap 的总数据量，单位为字节                                                   |
| svcNetIn              | int64  | 本地服务端口收到的网络流量，单位为字节                                        |
| svcNetOut             | int64  | 本地服务端口发送的网络流量，单位为字节                                        |
| shardNetIn            | int64  | shard 平面端口收到的网络流量，单位为字节                                       |
| shardNetOut           | int64  | shard 平面端口发送的网络流量，单位为字节                                       |
| replNetIn             | int64  | 数据同步平面端口收到的网络流量，单位为字节                                     |
| replNetOut            | int64  | 数据同步平面端口发送的网络流量，单位为字节                                     |
| SchdlrType            | int32  | 资源调度类型，取值如下：<br>0：没有开启资源调度<br>1：开启了 FIFO资 源调度 <br>2：开启了优先级资源调度<br>3：开启了基于容器的优先级资源调度                                     |
| SchdlrTypeDesp        | string | 资源调度类型描述，取值：NONE、FIFO、PRIORITY、CONTAINER                  |
| Run                   | int32  | 当前正在运行的任务数量                                                          |
| Wait                  | int32  | 当前处于等待队列的任务数量（包含未分发的任务）                                  |
| SchdlrMgrEvtNum       | int32  | 当前未分发的任务数量                                                            |
| SchdlrTimes           | int64  | 统计时间范围内总的任务执行次数                                                  |
| MemPoolSize           | int64  | Pool Memory 的大小，单位为字节                                                |

##示例##

查看数据库快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_DB" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:41000",
  "HostName": "hostname",
  "ServiceName": "41000",
  "GroupName": "group2",
  "IsPrimary": true,
  "ServiceStatus": true,
  "Status": "Normal",
  "FTStatus": "",
  "BeginLSN": {
    "Offset": -1,
    "Version": 0
  },
  "CurrentLSN": {
    "Offset": -1,
    "Version": 0
  },
  "CommittedLSN": {
    "Offset": -1,
    "Version": 0
  },
  "CompleteLSN": 0,
  "LSNQueSize": 0,
  "TransInfo": {
    "TotalCount": 0,
    "BeginLSN": -1
  },
  "NodeID": [
    1001,
    1001
  ],
  "Version": {
    "Major": 3,
    "Minor": 4,
    "Fix": 8,
    "Release": 43611,
    "GitVersion": "e0b89a77ec66e4ccecdc02b085029bca4c7aef4a",
    "Build": "2022-10-08-12.27.37(Enterprise Debug)"
  },
  "Edition": "Enterprise",
  "CurrentActiveSessions": 27,
  "CurrentIdleSessions": 9,
  "CurrentSystemSessions": 12,
  "CurrentTaskSessions": 6,
  "CurrentContexts": 1,
  "ReceivedEvents": 26,
  "Role": "data",
  "Disk": {
    "DatabasePath": "/opt/sequoiadb/database/data/11830/",
    "LoadPercent": 0,
    "TotalSpace": 754868568064,
    "FreeSpace": 622475874304
  },
  "TotalNumConnects": 0,
  "TotalQuery": 26,
  "TotalSlowQuery": 0,
  "TotalTransCommit": 0,
  "TotalTransRollback": 0,
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "ReplUpdate": 0,
  "ReplDelete": 0,
  "ReplInsert": 0,
  "TotalSelect": 0,
  "TotalRead": 0,
  "TotalLobGet": 0,
  "TotalLobPut": 0,
  "TotalLobDelete": 0,
  "TotalLobList": 0,
  "TotalLobReadSize": 0,
  "TotalLobWriteSize": 0,
  "TotalLobRead": 0,
  "TotalLobWrite": 0,
  "TotalLobTruncate": 0,
  "TotalLobAddressing": 0,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ActivateTimestamp": "2022-10-09-09.34.53.243408",
  "ResetTimestamp": "2022-10-09-09.34.53.243408",
  "UserCPU": "6.480000",
  "SysCPU": "4.630000",
  "freeLogSpace": 1342177280,
  "vsize": 2973097984,
  "rss": 52417,
  "fault": 0,
  "TotalMapped": 142868480,
  "svcNetIn": 0,
  "svcNetOut": 0,
  "shardNetIn": 11188,
  "shardNetOut": 102525,
  "replNetIn": 0,
  "replNetOut": 0,
  "SchdlrType": 0,
  "SchdlrTypeDesp": "NONE",
  "Run": 1,
  "Wait": 0,
  "SchdlrMgrEvtNum": 0,
  "SchdlrTimes": 0,
  "MemPoolSize": 100581376
}
...
```



[^_^]:
    本文使用的所有引用及链接
[architecture]:manual/Distributed_Engine/Architecture/Replication/architecture.md#全量同步
[regular_bar]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md