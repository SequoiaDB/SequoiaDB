[^_^]:

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见
    
    王涛：
    许建辉：
    市场部：

数据库快照可以列出数据库的状态和监控信息。

> Note:
>
> 协调节点通过聚合所有节点的数据（非协调节点字段信息）得到协调节点字段信息。用户可以通过 `coord.snapshot(SDB_SNAP_DATABASE,{RawData:true})` 获取聚合前的数据。


## 标识


SDB_SNAP_DATABASE

## 非协调节点字段信息

| 字段名                | 类型      | 描述                                                            |
| --------------------- | --------- | --------------------------------------------------------------- |
| NodeName              | string    | 节点名，格式为<主机名>:<服务名>                                 |
| HostName              | string    | 数据库的主机名                                                  |
| ServiceName           | string    | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标识                                                 |
| GroupName             | string    | 该逻辑节点所属的复制组名，standalone 模式下该字段为空字符串     |
| IsPrimary             | boolean   | 是否为主节点，standalone 模式下该字段为 false                   |
| Location              | string    | 节点的位置信息，该字段为空时表示未设置位置属性                  |
| IsLocationPrimary     | boolean   | 是否为位置集主节点                                              |
| ServiceStatus         | boolean   | 是否为可提供服务状态<br>一些特殊状态，例如[全量同步][replicate_url]时，服务状态为 false |
| Status                | string    | 节点状态，取值如下：<br/>            "Normal"：正常工作状态 <br/>              "Shutdown"：正在关闭状态，表示节点正在被关闭<br/>             "Rebuilding"：重新构建状态，如节点异常重启后，无法与其他节点进行数据同步时，节点会进入该状态，重新构建数据 <br/>           "FullSync"：全量同步状态 <br/>              "OfflineBackup"：[数据备份][regular_bar]状态   |
| FTStatus              | string | 容错状态，取值如下：<br> "NOSPC"：磁盘空间不足 <br>"DEADSYNC"：节点数据不同步 <br> "SLOWNODE"：节点数据同步过慢 <br> "TRANSERR"：节点事务异常 |
| BeginLSN.Offset       | int64 | 起始 LSN 的偏移                                                            |
| BeginLSN.Version      | int32   | 起始 LSN 的版本号                                                               |
| CurrentLSN.Offset     | int64 | 当前 LSN 的偏移                                                                 |
| CurrentLSN.Version    | int32   | 当前 LSN 的版本号                                                               |
| CommittedLSN.Offset   | int64 | 已提交 LSN 的偏移                                                               |
| CommittedLSN.Version  | int32   | 已提交 LSN 的版本号                                                             |
| CompleteLSN           | int64     | 已完成 LSN 的偏移                                               |
| LSNQueSize            | int32     | 等待同步的 LSN 队列长度                                           |
| TransInfo.TotalCount  | int32  | 正在执行的事务数量                                                              |
| TransInfo.BeginLSN    | int64 | 正在执行的事务的起始 LSN 的偏移                                                 |
| NodeID                | bson array| 节点的 ID 信息                                                  |
| Version.Major         | int32   | 数据库主版本号                                                                  |
| Version.Minor         | int32   | 数据库子版本号                                                                  |
| Version.Fix           | int32   | 数据库修复版本号                                                                |
| Version.Release       | int32   | 数据库内部版本号                                                                |
| Version.GitVersion    | string | 数据库发行版本号                                                                |
| Version.Build         | string | 数据库编译时间                                                                  |
| Edition               | string    | “Enterprise”表示企业版（社区版中无该字段）                           |
| CurrentActiveSessions | int32     | 当前活动会话                                                    |
| CurrentIdleSessions   | int32     | 当前非活动会话，一般来说非活动会话意味着 EDU 存在线程池中等待分配 |
| CurrentSystemSessions | int32     | 当前系统会话，为当前活动用户 EDU 数量                           |
| CurrentTaskSessions   | int32     | 后台任务会话数量                                                |
| CurrentContexts       | int32     | 当前上下文数量                                                  |
| ReceivedEvents        | int32     | 当前分区接收到的事件请求总数                                    |
| Role                  | string    | 当前节点角色                                                    |
| Disk.DatabasePath     | string    | 数据库所在路径                                                  |
| Disk.LoadPercent      | int32     | 数据库路径磁盘占用率百分比                                      |
| Disk.TotalSpace       | int64     | 数据库路径总空间，单位为字节                                    |
| Disk.FreeSpace        | int64     | 数据库路径空闲空间，单位为字节                                  |
| TotalNumConnects      | int32     | 数据库连接请求数量                                              |
| TotalQuery            | int64     | 总查询数量（广义查询，泛指在数据库上执行的所有操作）            |
| TotalSlowQuery        | int64     | 总慢查询数量（广义查询，泛指在数据库上执行的所有操作）          |
| TotalTransCommit      | int64     | 总事务提交数量                                                  |
| TotalTransRollback    | int64     | 总事务回滚数量                                                  |
| TotalDataRead         | int64     | 总数据读请求                                                    |
| TotalIndexRead        | int64     | 总索引读请求                                                    |
| TotalDataWrite        | int64     | 总数据写请求                                                    |
| TotalIndexWrite       | int64     | 总索引写请求                                                    |
| TotalUpdate           | int64     | 总更新记录数量                                                  |
| TotalDelete           | int64     | 总删除记录数量                                                  |
| TotalInsert           | int64     | 总插入记录数量                                                  |
| ReplUpdate            | int64     | 复制更新记录数量                                                |
| ReplDelete            | int64     | 复制删除记录数量                                                |
| ReplInsert            | int64     | 复制插入记录数量                                                |
| TotalSelect           | int64     | 总选择记录数量                                                  |
| TotalRead             | int64     | 总读取记录数量                                                  |
| TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效）      |
| TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效）      |
| TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效）      |
| TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效）      |
| TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效）      |
| TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效）      |
| TotalLobRead          | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效）         |
| TotalLobWrite         | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效）         |
| TotalLobTruncate      | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效）       |
| TotalLobAddressing    | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效）     |
| TotalReadTime         | int64     | 总读取时间，单位为毫秒                                          |
| TotalWriteTime        | int64     | 总写入时间，单位为毫秒                                          |
| ActivateTimestamp     | timestamp | 数据库启动时间                                                  |
| ResetTimestamp        | timestamp | 重置快照的时间                                                  |
| UserCPU               | double    | 用户 CPU，单位为秒                                              |
| SysCPU                | double    | 系统 CPU，单位为秒                                              |
| freeLogSpace          | int64     | 空闲日志空间，单位为字节                                        |
| vsize                 | int64     | 虚拟内存使用量，单位为字节                                      |
| rss                   | int64     | 物理内存使用量，单位为字节                                      |
| MemShared             | int64     | 物理内存中共享内存使用量，单位为字节                            |
| fault                 | int64     | 每秒访问失败数（仅支持 Linux），数据被交换出物理内存，放到 swap |
| TotalMapped           | int64     | mmap 的总数据量，单位为字节                                     |
| svcNetIn              | int64     | 本地服务端口收到的网络流量，单位为字节                          |
| svcNetOut             | int64     | 本地服务端口发送的网络流量，单位为字节                          |
| shardNetIn            | int64     | shard 平面端口收到的网络流量，单位为字节                        |
| shardNetOut           | int64     | shard 平面端口发送的网络流量，单位为字节                        |
| replNetIn             | int64     | 数据同步平面端口收到的网络流量，单位为字节                      |
| replNetOut            | int64     | 数据同步平面端口发送的网络流量，单位为字节                      |
| SchdlrType            | int32     | 资源调度类型，取值如下：<br>0：没有开启资源调度 <br>1：开启FIFO资源调度 <br> 2：开启优先级资源调度 <br>3：开启基于容器的优先级资源调度 |
| SchdlrTypeDesp        | string    | 资源调度类型描述，如：NONE、FIFO、PRIORITY、CONTAINER           |
| Run                   | int32     | 当前正在运行的任务数量                                          |
| Wait                  | int32     | 当前处于等待队列的任务数量（包含未分发的任务）                  |
| SchdlrMgrEvtNum       | int32     | 当前未分发的任务数量                                            |
| SchdlrTimes           | int64     | 统计时间范围内总的任务执行次数                                  |
| MemPoolSize           | int64     | Pool Memory 的大小，单位为字节                                  |
| MemPoolUsed           | int64     | Pool Memory 中被使用的大小，单位为字节                          |
| MemPoolFree           | int64     | Pool Memory 中空闲的大小，单位为字节                            |
| MemPoolMaxOOLSize     | int64     | 运行过程中超出 Pool Memory 的最大大小，单位为字节，表示 Pool Memory 需要调大 |


## 协调节点字段信息

| 字段名            | 类型      | 描述                                          |
| ----------------- | --------- | --------------------------------------------- |
| TotalNumConnects  | int32     | 数据库连接请求数量                            |
| TotalQuery        | int64     | 总查询数量（广义查询，泛指在数据库上执行的所有操作）  |
| TotalSlowQuery    | int64     | 总慢查询数量（广义查询，泛指在数据库上执行的所有操作）|
| TotalTransCommit  | int64     | 总事务提交数量                                     |
| TotalTransRollback| int64     | 总事务回滚数量                                     |
| TotalDataRead     | int64     | 总数据读请求                                  |
| TotalIndexRead    | int64     | 总索引读请求                                  |
| TotalDataWrite    | int64     | 总数据写请求                                  |
| TotalIndexWrite   | int64     | 总索引写请求                                  |
| TotalUpdate       | int64     | 总更新记录数量                                |
| TotalDelete       | int64     | 总删除记录数量                                |
| TotalInsert       | int64     | 总插入记录数量                                |
| ReplUpdate        | int64     | 复制更新记录数量                              |
| ReplDelete        | int64     | 复制删除记录数量                              |
| ReplInsert        | int64     | 复制插入记录数量                              |
| TotalSelect       | int64     | 总选择记录数量                                |
| TotalRead         | int64     | 总读取记录数量                                |
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
| TotalReadTime     | int64     | 总读取时间，单位为毫秒                        |
| TotalWriteTime    | int64     | 总写入时间，单位为毫秒                        |
| freeLogSpace      | int64     | 空闲日志空间，单位为字节                      |
| vsize             | int64     | 虚拟内存使用量，单位为字节                    |
| rss               | int64     | 物理内存使用量，单位为字节                    |
| MemShared         | int64     | 物理内存中共享内存使用量，单位为字节                            |
| fault             | int64     | 每秒访问失败数（仅支持 Linux），数据被交换出物理内存，放到 swap |
| TotalMapped       | int64     | mmap 的总数据量，单位为字节                   |
| svcNetIn          | int64     | 本地服务端口收到的网络流量，单位为字节        |
| svcNetOut         | int64     | 本地服务端口发送的网络流量，单位为字节        |
| shardNetIn        | int64     | shard 平面端口收到的网络流量，单位为字节      |
| shardNetOut       | int64     | shard 平面端口发送的网络流量，单位为字节      |
| replNetIn         | int64     | 数据同步平面端口收到的网络流量，单位为字节    |
| replNetOut        | int64     | 数据同步平面端口发送的网络流量，单位为字节    |
| MemPoolSize       | int64     | Pool Memory 的大小，单位为字节                                  |
| MemPoolUsed       | int64     | Pool Memory 中被使用的大小，单位为字节                          |
| MemPoolFree       | int64     | Pool Memory 中空闲的大小，单位为字节                            |
| MemPoolMaxOOLSize | int64     | 运行过程中超出 Pool Memory 的最大大小，单位为字节，表示 Pool Memory 需要调大 |
| ErrNodes          | bson array| 异常节点的信息         |
| ErrNodes.NodeName | string    | 异常节点名，格式为<主机名>:<服务名>                     |
| ErrNodes.GroupName| string    | 异常节点所属复制组名                                    |
| ErrNodes.Flag     | int32     | 异常节点的[错误码][error_code_url]                      |
| ErrNodes.ErrInfo  | bson      | 异常节点的错误信息                                      |

> **Note:**
>
> 当存在异常节点时才显示 ErrNodes 字段。

## 示例

- 通过非协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_DATABASE )
   ```

   输出结果如下：
   ```lang-json
   {
     "NodeName": "hostname1:11820",
     "HostName": "hostname1",
     "ServiceName": "11820",
     "GroupName": "group1",
     "IsPrimary": true,
     "Location": "GuangZhou",
     "IsLocationPrimary": true,
     "ServiceStatus": true,
     "Status": "Normal",
     "FTStatus": "",
     "BeginLSN": {
       "Offset": 0,
       "Version": 1
     },
     "CurrentLSN": {
       "Offset": 164,
       "Version": 1
     },
     "CommittedLSN": {
       "Offset": 164,
       "Version": 1
     },
     "CompleteLSN": 1468,
     "LSNQueSize": 0,
     "TransInfo": {
       "TotalCount": 0,
       "BeginLSN": -1
     },
     "NodeID": [
       1000,
       1000
     ],
     "Version": {
       "Major": 3,
       "Minor": 4,
       "Fix": 8,
       "Release": 43597,
       "GitVersion": "6c1922758c5d89ed124f4326122fc32d2a468505",
       "Build": "2022-10-06-10.43.42(Enterprise Debug)"
     },
     "Edition": "Enterprise",
     "CurrentActiveSessions": 26,
     "CurrentIdleSessions": 10,
     "CurrentSystemSessions": 12,
     "CurrentTaskSessions": 6,
     "CurrentContexts": 1,
     "ReceivedEvents": 16,
     "Role": "data",
     "Disk": {
       "DatabasePath": "/opt/sequoiadb/database/data/11820/",
       "LoadPercent": 0,
       "TotalSpace": 754868568064,
       "FreeSpace": 628498378752
     },
     "TotalNumConnects": 2,
     "TotalQuery": 14,
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
     "TotalLobPut": 1,
     "TotalLobDelete": 0,
     "TotalLobList": 0,
     "TotalLobReadSize": 0,
     "TotalLobWriteSize": 150,
     "TotalLobRead": 0,
     "TotalLobWrite": 1,
     "TotalLobTruncate": 0,
     "TotalLobAddressing": 1,
     "TotalReadTime": 0,
     "TotalWriteTime": 0,
     "ActivateTimestamp": "2022-10-06-18.00.07.588438",
     "ResetTimestamp": "2022-10-06-18.00.07.588438",
     "UserCPU": "0.730000",
     "SysCPU": "2.170000",
     "freeLogSpace": 1342177280,
     "vsize": 3302031360,
     "rss": 56151,
	 "MemShared": 16121,
     "fault": 0,
     "TotalMapped": 667385856,
     "svcNetIn": 468,
     "svcNetOut": 6185,
     "shardNetIn": 9939,
     "shardNetOut": 9454,
     "replNetIn": 0,
     "replNetOut": 0,
     "SchdlrType": 0,
     "SchdlrTypeDesp": "NONE",
     "Run": 0,
     "Wait": 0,
     "SchdlrMgrEvtNum": 0,
     "SchdlrTimes": 0,
     "MemPoolSize": 108975104,
     "MemPoolUsed": 68871074,
     "MemPoolFree": 40104030,
     "MemPoolMaxOOLSize": 0,
   }
   ```

- 通过协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_DATABASE )
   ```

   输出结果如下：
   ```lang-json
   {
     "TotalNumConnects": 1,
     "TotalQuery": 18,
     "TotalSlowQuery": 0,
     "TotalTransCommit": 21,
     "TotalTransRollback": 0,
     "TotalDataRead": 682,
     "TotalIndexRead": 561,
     "TotalDataWrite": 27,
     "TotalIndexWrite": 24,
     "TotalUpdate": 19,
     "TotalDelete": 0,
     "TotalInsert": 8,
     "ReplUpdate": 0,
     "ReplDelete": 0,
     "ReplInsert": 0,
     "TotalSelect": 307,
     "TotalRead": 403,
     "TotalLobGet": 0,
     "TotalLobPut": 2,
     "TotalLobDelete": 0,
     "TotalLobList": 1,
     "TotalLobReadSize": 0,
     "TotalLobWriteSize": 300,
     "TotalLobRead": 0,
     "TotalLobWrite": 1,
     "TotalLobTruncate": 0,
     "TotalLobAddressing": 1,
     "TotalReadTime": 0,
     "TotalWriteTime": 0,
     "freeLogSpace": 5368709120,
     "vsize": 13657559040,
     "rss": 257087,
	 "MemShared": 116124,
     "fault": 9,
     "TotalMapped": 1377402880,
     "svcNetIn": 1536,
     "svcNetOut": 1057,
     "shardNetIn": 33090,
     "shardNetOut": 22375,
     "replNetIn": 0,
     "replNetOut": 0,
     "MemPoolSize": 108975104,
     "MemPoolUsed": 68871074,
     "MemPoolFree": 40104030,
     "MemPoolMaxOOLSize": 0,
     "ErrNodes": []
   }
   ```

[^_^]:
    本文使用到的所有链接及引用。

[replicate_url]: manual/Distributed_Engine/Architecture/Replication/architecture.md
[error_code_url]: manual/Manual/Sequoiadb_error_code.md
[regular_bar]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md