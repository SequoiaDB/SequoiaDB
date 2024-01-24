[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见
    
    王涛：
    许建辉：
    市场部：



操作系统快照可以列出操作系统的状态和监控信息。

> Note:
>
> 协调节点通过聚合所有节点的数据（非协调节点字段信息）得到协调节点字段信息。用户可以通过 `coord.snapshot(SDB_SNAP_SYSTEM,{RawData:true})` 获取聚合前的数据。


## 标识

SDB_SNAP_SYSTEM

## 非协调节点字段信息

| 字段名               | 类型      |  描述                                                          |
| -------------------- | --------- | -------------------------------------------------------------- |
| NodeName             | string    | 节点名，格式为<主机名>:<服务名>                                |
| HostName             | string    | 数据库的主机名                                                 |
| ServiceName          | string    | 数据库的服务名                                                 |
| GroupName            | string    | 该逻辑节点所属的复制组名，standalone 模式下该字段为空字符串    |
| IsPrimary            | boolean   | 是否为主节点，standalone 模式下该字段为 false                  |
| Location              | string    | 节点的位置信息，该字段为空时表示未设置位置属性                  |
| IsLocationPrimary     | boolean   | 是否为位置集主节点                                              |
| ServiceStatus        | boolean   | 是否为可提供服务状态 <br>一些特殊状态，例如[全量同步][replicate_url]时，服务状态为 false |
| Status               | string    | 数据库状态，如：Normal、Shutdown、Rebuilding、FullSync、OfflineBackup |
| BeginLSN.Offset      | int64 | 起始 LSN 的偏移                                                |
| BeginLSN.Version     | int32   | 起始 LSN 的版本号                                              |
| CurrentLSN.Offset    | int64 | 当前 LSN 的偏移                                                |
| CurrentLSN.Version   | int32   | 当前 LSN 的版本号                                              |
| CommittedLSN.Offset  | int64 | 已提交 LSN 的偏移                                              |
| CommittedLSN.Version | int32   | 已提交 LSN 的版本号                                            |
| CompleteLSN          | int64     | 已完成 LSN 的偏移                                              |
| LSNQueSize           | int32     | 等待同步的LSN队列长度                                          |
| TransInfo.TotalCount | int32   | 正在执行的事务数量                                             |
| TransInfo.BeginLSN   | int64 | 正在执行的事务的起始 LSN 的偏移                                |
| NodeID               | bson array| 节点的 ID 信息                                                 |
| CPU.User             | double | 操作系统启动后所消耗的总用户 CPU 时间，单位为秒              |
| CPU.Sys              | double | 操作系统启动后所消耗的总系统 CPU 时间，单位为秒              |
| CPU.Idle             | double | 操作系统启动后所消耗的总空闲 CPU 时间，单位为秒              |
| CPU.Other            | double | 操作系统启动后所消耗的总其它 CPU 时间，单位为秒              |
| Memory.LoadPercent   | int32   | 当前操作系统的内存使用百分比（包括文件系统缓存）               |
| Memory.TotalRAM      | int64 | 当前操作系统的总内存空间，单位为字节                         |
| Memory.FreeRAM       | int64 | 当前操作系统的空闲内存空间，单位为字节                       |
| Memory.TotalSwap     | int64 | 当前操作系统的总交换空间，单位为字节                          |
| Memory.FreeSwap      | int64 | 当前操作系统的空闲交换空间，单位为字节                        |
| Memory.TotalVirtual  | int64 | 当前操作系统的总虚拟空间，单位为字节                         |
| Memory.FreeVirtual   | int64 | 当前操作系统的空闲虚拟空间，单位为字节                        |
| Disk.Name            | string | 数据库路径所在的磁盘名称<br>                                   |
| Disk.DatabasePath    | string | 数据库路径                                                     |
| Disk.LoadPercent     | int32   | 数据库路径所在文件系统的空间占用百分比                         |
| Disk.TotalSpace      | int64 | 数据库路径总空间，单位为字节                                 |
| Disk.FreeSpace       | int64 | 数据库路径空闲空间，单位为字节                               |


## 协调节点字段信息

| 字段名      | 类型   |  描述                            |
| ----------- | ------ | -------------------------------- |
| CPU.User        | double | 操作系统启动后所消耗的总用户 CPU 时间，单位为秒    |
| CPU.Sys         | double | 操作系统启动后所消耗的总系统 CPU 时间，单位为秒    |
| CPU.Idle        | double | 操作系统启动后所消耗的总空闲 CPU 时间，单位为秒        |
| CPU.Other       | double | 操作系统启动后所消耗的总其它 CPU 时间，单位为秒    |
| Memory.TotalRAM      | int64  | 当前操作系统的总内存空间，单位为字节        |
| Memory.FreeRAM       | int64  | 当前操作系统的空闲内存空间，单位为字节      |
| Memory.TotalSwap     | int64  | 交换分区的总空间，单位为字节    |
| Memory.FreeSwap      | int64  | 当前操作系统的总交换空间，单位为字节  |
| Memory.TotalVirtual  | int64  | 当前操作系统的总虚拟空间，单位为字节    |
| Memory.FreeVirtual   | int64  | 当前操作系统的空闲虚拟空间，单位为字节  |
| Disk.TotalSpace      | int64  | 数据路径下的总存储空间，单位为字节     |
| Disk.FreeSpace       | int64  | 数据路径下的空闲存储空间，单位为字节   |
| ErrNodes.NodeName | string    | 异常节点名，格式为<主机名>:<服务名>                    |
| ErrNodes.GroupName| string    | 异常节点所属复制组名                                   |
| ErrNodes.Flag     | int32     | 异常节点的[错误码][error_code_url]                     |
| ErrNodes.ErrInfo  | bson      | 异常节点的错误信息                                     |


## 示例

- 通过非协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_SYSTEM )
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "hostname1:11820",
     "HostName": "hostname1",
     "ServiceName": "11820",
     "GroupName": "group1",
     "IsPrimary": false,
     "Location": "GuangZhou",
     "IsLocationPrimary": false,
     "ServiceStatus": true,
     "Status": "Normal",
     "BeginLSN": {
       "Offset": 0,
       "Version": 1
     },
     "CurrentLSN": {
       "Offset": 3764,
       "Version": 1
     },
     "CommittedLSN": {
       "Offset": 3764,
       "Version": 1
     },
     "CompleteLSN": 3865,
     "LSNQueSize": 0,
     "TransInfo": {
       "TotalCount": 0,
       "BeginLSN": -1
       },
     "NodeID": [
       1000,
       1000
     ],
     "CPU": {
       "User": 3947.31,
       "Sys": 715.11,
       "Idle": 331196.41,
       "Other": 771.14
     },
     "Memory": {
       "LoadPercent": 95,
       "TotalRAM": 4155072512,
       "FreeRAM": 202219520,
       "TotalSwap": 2153771008,
       "FreeSwap": 2137071616,
       "TotalVirtual": 6308843520,
       "FreeVirtual": 2339291136
     },
     "Disk": {
       "Name":"/dev/sda1",
       "DatabasePath": "/opt/sequoiadb/database/data/11820",
       "LoadPercent": 78,
       "TotalSpace": 40704466944,
       "FreeSpace": 8615747584
     }
   }
   ```

- 通过协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_SYSTEM )
   ```

   输出结果如下：

   ```lang-json
   {
     "CPU": {
       "User": 36280.72,
       "Sys": 5046.23,
       "Idle": 7560242.4,
       "Other": 5887.24
     },
     "Memory": {
       "TotalRAM": 8403730432,
       "FreeRAM": 3075035136,
       "TotalSwap": 25757204480,
       "FreeSwap": 25663799296,
       "TotalVirtual": 34160934912,
       "FreeVirtual": 28738834432
     },
     "Disk": {
       "TotalSpace": 338172772352,
       "FreeSpace": 181331296256
     },
     "ErrNodes": [
       {
         "NodeName": "hostname1:11850",
         "GroupName": "group2",
         "Flag": -79,
         "ErrInfo": {}
       }
     ]
   }
   ```

[^_^]:
    本文使用到的所有链接及引用。

[replicate_url]: manual/Distributed_Engine/Architecture/Thread_Model/edu.md
[error_code_url]: manual/Manual/Sequoiadb_error_code.md