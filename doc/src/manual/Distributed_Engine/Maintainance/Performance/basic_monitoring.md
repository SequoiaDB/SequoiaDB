[^_^]: 
    单机性能监控
    作者：李元锴
    时间：20190425
    评审意见
    王涛：
    许建辉：
    市场部：20190801



本文档将介绍 SequoiaDB 巨杉数据库提供的单节点监控功能，包含集群内单个节点的性能及状态信息。了解监控是数据库管理的重要组成部分，能为评估数据库当前状况和性能调优提供帮助。

监控命令
----

在 SequoiaDB 中，用户可以使用[快照][SNAPSHOT]命令进行简单监控。快照命令中与数据库单机性能相关的快照类型有如下几种：

- 数据库快照：数据库节点级别的统计信息，反映了数据库会话状态，网络通讯流量，以及当前实例启动以来各类操作的计数
- 事务快照：数据库节点中正在运行的事务状态，包括事务执行状态，持有及等待的锁信息

热点会话
----

- [sdbtop][sdbtop] 工具可以使用如下命令查看当前热点会话列表：

   ```lang-bash
   sdbtop -i server1 -s 11820 -u sdbadmin -p sdbadmin
   ```

- 进入 sdbtop 统计界面后，输入 `s`，进入会话统计界面；输入两次 `TAB` 键，切换到平均值模式；输入 `D` 并输入排序的列，统计界面将以输入的列降序排序，列出热点会话。

 例如：输入需要排序的列 DataRead，统计界面动态展示当前节点数据读最高的若干会话

   ```lang-text
   refresh= 3 secs                                                                              version {version}                                                                         snapshotMode: GLOBAL
   displayMode: AVERAGE                                                                          Sessions                                                                         snapshotModeInput: NULL
   hostname: localhost                                                                                                                                                                 filtering Number: 0
   servicename: 11810                                                                                                                                                  sortingWay: NULL sortingField: NULL
   usrName: NULL                                                                                                                                                             Refresh: F5, Quit: q, Help: h
   
        SessionID                           TID Type               Name                            QueueSize  ProcessEventCount Contexts              DataRead       IndexRead      
        ------------------------------ -------- ------------------ ------------------------------ ---------- ------------------ --------------- -------------- --------------- 
     1  1                                  2252 Task               DATASYNC-JOB-D                          0                  1 []                           2               0      
     2  1                                  2251 Task               DATASYNC-JOB-D                          0                  1 []                           1               0 
   ```

用户活动连接数
----

数据库快照可以查看当前节点各类连接数

```lang-javascript
> db.snapshot(SDB_SNAP_DATABASE)
```

输出结果如下：

```lang-json
{
  ……
  "CurrentActiveSessions": 19,
  "CurrentIdleSessions": 9,
  "CurrentSystemSessions": 12,
  "CurrentTaskSessions": 5,
  ……
}
```

活动连接数反映了当前节点的任务数量。通过该类指标，可以评估当前节点的系统任务和用户任务的运行负载。

- CurrentActiveSessions：活动会话数量，该值记录了当前正在运行和阻塞的任务数量，包括系统任务和用户任务

- CurrentIdleSessions：非活动会话数量，由于线程的创建和销毁开销是巨大的，将使用完的会话线程保存在线程池中进行复用，减少了这些不必要的开销，非活动会话指的就是保存在线程池中的空闲会话线程

- CurrentSystemSessions：系统会话数量，该值记录了当前正在运行和阻塞的任务数量，不包括用户任务

- CurrentTaskSessions：后台处理任务数量，返回值中Type为Task的任务

读写操作量
----

数据库快照可以查看当前节点读写操作

```lang-javascript
> db.snapshot(SDB_SNAP_DATABASE)
```

输出结果如下：

```lang-json
{
  ……
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 56,
  "TotalRead": 0,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  ……
}
```

当前节点从启动实例以来，对数据库的各类操作的计数器，这些数字将随着时间的推移而增长。

- TotalDataRead、TotalIndexRead、TotalDataWrite、TotalIndexWrite：数据和索引读写计数，各类操作计数记录读取或写入操作，访问数据或索引的次数。重点反映数据库对用户请求的实际执行情况。如果读指标 TotalDataRead 远大于 TotalIndexRead，表明查询是通过表扫描检索数据，需对比查询条件和已有索引是否匹配，或者添加新的索引。

- TotalRead、TotalSelect、TotalUpdate、TotalDelete、TotalInsert：各类请求操作计数，各类操作计数记录用户读写请求的次数，重点反映数据库接收到用户的各类请求。如果读指标 TotalRead 与 TotalSelect 相当，表明查询命中效率较高，读取性能最好，否则可能执行表扫描。其中，TotalInsert、TotalUpdate 与 TotalDelete 的值包含 ReplInsert、ReplUpdate 和 ReplDelete。

- TotalReadTime、TotalWriteTime：读写操作耗时，记录用户读写请求的耗时

副本集同步
----

数据库快照可以查看当前节点副本集同步性能

```lang-javascript
> db.snapshot(SDB_SNAP_DATABASE)
```

输出结果如下：

```lang-json
{
  ……
  "ReplUpdate": 0,
  "ReplDelete": 0,
  "ReplInsert": 0，
  "replNetIn": 0,
  "replNetOut": 0,
  ……
}
```

- ReplUpdate、ReplDelete、ReplInsert：主节点写操作同步至备节点计数，各类写操作计数记录副本同步写操作数量。节点写入操作量大时，该值会同时增加

- replNetIn、replNetOut：主节点写操作同步至备节点网络流量信息，重点关注备节点上的replNetIn和主节点的replNetOut，网络带宽压力大时，可以看出副本同步占用的带宽比例

网络流量
----

数据库快照可以查看当前节点各类网络流量

```lang-javascript
> db.snapshot(SDB_SNAP_DATABASE)
```

输出结果如下：

```lang-json
{
  ……
  "svcNetIn": 546,
  "svcNetOut": 20495,
  "shardNetIn": 9956,
  "shardNetOut": 46310,
  ……
}
```

- svcNetIn、svcNetOut：节点直连服务占用的网络流量，网络带宽压力大时，可以看出直连服务占用的带宽比例

- shardNetIn、shardNetOut：节点分区服务占用的网络流量，网络带宽压力大时，可以看出分区服务占用的带宽比例

资源调度
----

数据库快照可以查看当前节点资源调度使用情况

```lang-javascript
> db.snapshot(SDB_SNAP_DATABASE)
```

输出结果如下：

```lang-json
{
  ……
  "Run": 0,
  "Wait": 0,
  ……
}
```

该指标只有开启了资源调度功能才有效，该类值记录了本节点上的任务运行情况，判断节点的任务执行情况。

- Run：当前正在运行的任务数量，该指标是实时值，最大值受节点配置参数 svcmaxconcurrency 的限制

- Wait：当前处于等待队列的任务数量，当节点上正在运行的任务数量达到最大值是，后续的任务需要排队等待，该值反映了排队等待的任务数

锁相关
----

事务快照可以查看当前锁使用情况

```lang-javascript
> db.snapshot(SDB_SNAP_TRANSACTIONS)
```

输出结果如下：

```lang-json
{
  ……
  "WaitLock": {
    "CSID": -1,
    "CLID": 65535,
    "recordID": -1,
    "recordOffset": -1
  },
  "TransactionLocksNum": 3,
  "GotLocks": [
    {
      "CSID": 1,
      "CLID": 0,
      "recordID": -1,
      "recordOffset": -1
    }
  ],
  ……
}
```

- WaitLock：事务等待获取的锁

- GotLocks：事务持有的锁

- 锁对象内容说明

  |锁对象|CSID|CLID|recordID|recordOffset|
  |----|----|----|----|----|
  |没有锁|-1|65535|-1|-1|
  |集合空间锁|>= 0|65535|-1|-1|
  |集合锁|>= 0|>= 0|-1|-1|
  |行锁|>= 0|>= 0|>= 0|>= 0|


[^_^]:
     本文所有引用和链接
[SNAPSHOT]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
[sdbtop]:manual/Distributed_Engine/Maintainance/Monitoring/sdbtop.md