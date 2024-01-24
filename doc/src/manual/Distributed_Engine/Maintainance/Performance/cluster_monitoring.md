本文档介绍 SequoiaDB 巨杉数据库提供的集群监控功能，用于快速找到数据库实例性能和活动的统计信息。

##客户端连接数##

客户端连接数反映了应用系统与集群建立的连接数量。

查看客户端连接数

```lang-bash
$ sdb -f getCoordConn.js
```

`getCoordConn.js` 内容如下：

```lang-javascript
var db = new Sdb();
var nodes = db.list(SDB_LIST_GROUPS, {GroupName: "SYSCoord"}, {"Group.HostName": 1, "Group.Service.Name": 1}).next().toObj()["Group"];
var sum = 0;
for(var i in nodes){
   var node = nodes[i];
   sum += new Sdb(node["HostName"], node["Service"][0]["Name"]).snapshot(6, {}, {"TotalNumConnects": 1}).next().toObj()["TotalNumConnects"];
}
println("集群客户端连接数："+sum);
```

连接会消耗文件句柄和内存资源，并发量过大还会导致线程上下文的频繁切换。如果该指标高于预期值，表明用户请求量超过了数据库处理请求的能力；当数据节点压力不大时，可以增加协调节点提高并发量，否则需要对集群进行扩容。

数据库连接的创建是比较耗时的操作，应用程序频繁创建销毁，未关闭连接也有可能导致连接数过高，建议使用驱动程序中的数据库连接池进行连接管理，连接池允许应用程序更有效地使用和重用连接。

##读写操作量##

集群读写操作量连接的是集群协调节点，输出内容是整个集群的读写操作数量。

- DataRead 和 IndexRead 

   数据和索引的访问次数，如果指标值偏低，建议应用配置会话为读写分离模式，如果 DataRead 值远高于 IndexRead 的值，可能会话中有未通过索引检索，直接进行表扫描的查询，通过使用 sdbtop 工具的会话统计界面，查出 DataRead 高的会话, 检查该查询是否有命中的索引。其次，查看该查询的访问计划，查看用户请求在每个数据节点的索引命中情况和返回记录条数，如果在某些节点的 ScanType 为 tbscan ，则有可能是部分节点索引缺失导致未命中索引，如果各节点返回记录条数差距过大，则有可能是分区键选择不合理，导致热点数据。

- DataWrite和 IndexWrite
 
   数据和索引的写入次数，如果指标值偏低，检查应用程序，确认能否将单条插入改写成批量插入的方式。其次如果观察到数据节点磁盘 I/O 偏高，建议增加集合的分片数量，将 I/O 压力分担到其他磁盘。或考虑使用 SSD 磁盘替换 SAS 或 SATA 盘。

##副本集同步##

snapshot 命令可以查看当前节点副本集同步状态

```lang-javascript
> db.snapshot(SDB_SNAP_HEALTH, {}, {LSNQueSize: 1, DiffLSNWithPrimary: 1})
```

输出结果如下：

```lang-json
{
  "LSNQueSize": 0,
  "DiffLSNWithPrimary": 0
}
{
  "LSNQueSize": 0,
  "DiffLSNWithPrimary": 0
}
{
  "LSNQueSize": 0,
  "DiffLSNWithPrimary": 0
}
```

- LSNQueSize：等待同步的 LSN 队列长度
- DiffLSNWithPrimary：与主节点的 LSN 差异

通过以上两个指标，可以看出集群数据复制组间副本集数据同步的压力，如果两个指标长期较高，需要检查复制组间网络状态。

##节点运行状态##

snapshot 命令可以查看当前节点运行状态

```lang-javascript
> db.snapshot(SDB_SNAP_HEALTH, {}, {ServiceStatus: 1, SyncControl: 1})
```

输出结果如下：

```lang-json
{
  "ServiceStatus": true,
  "SyncControl": false
}
{
  "ServiceStatus": true,
  "SyncControl": false
}
{
  "ServiceStatus": true,
  "SyncControl": false
}
```

通过以上两个指标，可以看出集群节点的运行状态。如果 ServiceStatus 为 false，表明节点状态异常，不对外提供服务；如果 SyncControl 为 true，表明节点正在进行数据全量同步。