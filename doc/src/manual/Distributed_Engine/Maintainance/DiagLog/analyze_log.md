用户在使用 SequoiaDB 巨杉数据库的过程中，如果遇到一些由数据库抛出的异常错误，可以通过 [sdbdiag.log][diaglog] 日志来定位问题。在进行集群日志分析前，用户可参考 SequoiaDB 的[节点][node]章节，了解 SequoiaDB 中各类节点在请求执行中的作用。

diaglog问题诊断
----

用户在使用 SequoiaDB 集群时，如果遇到命令执行失败或者查询数据抛出异常，`sdbdiag.log` 日志能够帮助用户快速定位具体失败原因。例如，当用户向数据库进行数据写入时，数据无法写入导致写入操作执行失败。具体执行命令如下：

```lang-bash
> db.sample.employee.insert({"id": 1001, "name": "zhangSan"})
sdb.js:625 uncaught exception: -105
Not enough number of data nodes
Takes 0.006951s.
```

SequoiaDB 处理请求的过程：协调节点接收客户端的请求并转发至对应的数据节点，数据节点将执行结果返回至协调节点后，由协调节点汇总并返回至客户端。

用户在进行日志分析时，应先分析协调节点日志，再分析数据节点日志。以下是具体分析步骤：

1. 查看[协调节点][coord_node]的诊断日志，查看方式可参考[节点诊断日志][diaglog]章；通过分析可以看到记录在插入 “GroupID:1000, NodeID:1001” 节点时失败

   ```lang-text
   ...
   2019-07-05-13.38.41.145166               Level:ERROR
   PID:7038                                 TID:7507
   Function:execute                         Line:341
   File:SequoiaDB/engine/coord/coordInsertOperator.cpp
   Message:
   Insert failed on node[{ GroupID:1000, NodeID:1001, ServiceID:2(SHARD) }], rc: -105
   
   2019-07-05-13.38.41.146227               Level:ERROR
   PID:7038                                 TID:7507
   Function:processMsg                      Line:1869
   File:SequoiaDB/engine/pmd/pmdProcessor.cpp
   Message:
   Error processing Agent request, rc=-105
   ...
   ```
  
   > **Note：**
   >
   > 每个协调节点只会记录通过自身连接进行请求的相关信息，用户需要确认请求通过哪个协调节点发出，以便于找到正确的日志信息。


2. 通过[复制组列表][SDB_LIST_GROUPS]获取异常节点 “GroupID:1000, NodeID:1001” 信息

   ```lang-bash
   $ sdb 'db.list(SDB_LIST_GROUPS, {"GroupID" : 1000})'
   ```

   由输出结果可以判断发生异常的节点是 `sdbserver2:11820`

   ```lang-json
   ...
   {
     "HostName": "sdbserver2",
     "Status": 1,
     "dbpath": "/opt/sequoiadb/database/data/11820/",
     "Service": [
       {
         "Type": 0,
         "Name": "11820"
       },
       {
         "Type": 1,
         "Name": "11821"
       },
       {
         "Type": 2,
         "Name": "11822"
       }
     ],
     "NodeID": 1001
   }
   ...
   ```

3. 查看[数据节点][data_node]的诊断日志，通过分析判断在连接 `sdbserver3:11821` 时失败，而 11821 端口被 11820 节点占用；节点的当前存活数量为：2，需要存活的节点数量为：3

   ```lang-text
   ...
   2019-07-05-13.38.27.749514               Level:ERROR
   PID:7046                                 TID:7336
   Function:syncConnect                     Line:358
   File:SequoiaDB/engine/net/netEventHandler.cpp
   Message:
   Connection[Handle:8] connect to sdbserver3:11821(11821) failed, rc: -79
   
   2019-07-05-13.38.27.749565               Level:EVENT
   PID:7046                                 TID:7336
   Function:_sharingBeat                    Line:866
   File:SequoiaDB/engine/cls/clsReplicateSet.cpp
   Message:
   Reset node[1005] sharing-beat time to 1800(sec)
   
   2019-07-05-13.38.41.142127               Level:ERROR
   PID:7046                                 TID:7554
   Function:_calculateW                     Line:4740
   File:SequoiaDB/engine/cls/clsShardSession.cpp
   Message:
   alive num[2] can not meet need[3]
   ...
   ```
  
4. 查看集合的编目信息快照，通过分析可以判断集合的的同步副本数 ReplSize 设置为 3，即要求数据在写入三个副本后才返回成功

   ```lang-bash
  > db.snapshot(SDB_SNAP_CATALOG, {"Name":"emp.employees"})
  {
    ...
    "Name": "sample.employees",
    "UniqueID": 17179869185,
    "Version": 2,
    "ReplSize": 3,
    "Attribute": 0,
    "AttributeDesc": "",
    "ShardingKey": {
      "_id": 1
    }
    ...
  }
```

根据以上分析，可以得出结论：

- 在数据插入时，数据只成功写入到两个副本，与该集合要求的三个副本不符，导致数据插入失败而抛出异常。
- 数据插入失败的节点是 sdbserver3 机器的 11820 节点，说明 sdbserver3 机器的 11820 节点下线。


[^_^]:
     本文使用的所有引用和链接
[diaglog]:manual/Distributed_Engine/Maintainance/DiagLog/diaglog.md
[coord_node]:manual/Distributed_Engine/Architecture/Node/coord_node.md
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[SDB_LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
[node]:manual/Distributed_Engine/Architecture/Node/Readme.md
