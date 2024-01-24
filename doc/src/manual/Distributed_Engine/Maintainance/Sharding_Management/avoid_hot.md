[^_^]:
    避免热点数据
    作者：魏彰凯
    时间：20191001
    评审意见
    王涛：
    许建辉：
    市场部：20190816


短时间内被频繁访问的数据称为热点数据。避免热点数据，可以有效利用数据库的读写性能，优化数据分布。在 SequoiaDB 巨杉数据库中，某个数据组或者主数据节点繁忙，证明出现了热点数据。

- 某个数据组繁忙：某个数据组繁忙程度远高于其它数据组时，代表大量的数据操作出现在该数据组上，需要将当前数据组上的数据切分到其他数据组，让其他数据组共同承担读写压力
- 主数据节点繁忙：主数据节点繁忙程度远高于同组内备数据节点时，代表当前大量的数据操作为查询操作，可以将部分查询操作分散到备数据节点

某个数据组繁忙
----

某个数据组繁忙程度远高于其他数据组时，表示集合内的数据分布不合理。在 SequoiaDB 中，集合内的数据，会通过 hash 和 range 两种方式切分到不同数据组上，从而避免热点数据。

### hash 分区 ###

集合采用 hash 分区方式时，会对[分区键][sharding_key]字段进行路由运算，确定数据的落点。理论上这种分区方式数据可以保证数据均衡分布。

集合采用 hash 分区方式时，出现热点问题，代表了某一个 hash 值大量出现，即分区键上重复的数据较多。这种情况下需要重新选择分区键，对集合进行重构。

假如集合 avoid_hot.user_info 存在如下数据，用户需要将集合作为 hash 分区集合存入巨杉数据库中：

| id | name | gender | addr | email | date |
|:----:|:------:|:-----:|:------:|:-------:|:------:|
|U00001|Celia|F|北京|celia@sina.cn|2019-05-30|
|U00002|Haley|F|上海|haley@sina.cn|2019-05-31|
|U00003|Byrne|M|深圳|byrne@sina.cn|2019-05-31|
|U00004|Caleb|M|北京|caleb@sina.cn|2019-05-31|
|U00005|Haley|F|北京|haley@sina.cn|2019-05-31|

创建集合时选择 date 字段作为分区键，插入“2019-05-31”的数据时，所有的数据操作将集中在一个数据组中
```lang-bash
> db.createCS("avoid_hot")
> db.avoid_hot.createCL( "user_info", { ShardingKey: { date: 1 }, ShardingType: "hash", AutoSplit: true } )
```

**处理方案**

- 以 id 做分区键，创建新集合
 
   ```lang-javascript
   > db.avoid_hot.createCL( "user_info_new", { ShardingKey: { id: 1 }, ShardingType: "hash", AutoSplit: true } )
   ```

- 导出原有集合中的数据

   ```lang-bash
   $ sdbexprt -s localhost -p 11810 --type csv --file avoid_hot.user_info.csv -c avoid_hot -l user_info
   ```

- 将导出的数据导入新集合

   ```lang-bash
   $ sdbimprt --hosts=localhost:11810 --type=csv --file=avoid_hot.user_info.csv -c avoid_hot -l user_info_new
   ```

- 修改表名，用新集合替代原有集合

   ```lang-javascript
   > db.avoid_hot.renameCL( "user_info", "user_info_bak")
   > db.avoid_hot.renameCL( "user_info_new", "user_info")
     ```

### range 分区 ###

集合采用 range 分区方式时，会对分区键字段进行判断，将指定范围的数据放入指定数据组中。这种分区方式长期来看可以保证数据均衡分布，短期内对分区键连续操作时，容易出现热点问题。

集合采用 range 分区方式时，出现热点问题，需要将数据随机放入不同的数据组中，避免热点问题。在 SequoiaDB 中，可以使用[多维分区][multi_dimension]方式解决该问题。

**avoid_hot.user_info 集合的多维分区方式**

1. 创建主表 avoid_hot.user_info

   ```lang-javascript
   > db.avoid_hot.createCL( "user_info", { ShardingKey: { date: 1 }, ShardingType: "range", IsMainCL: true } )
   ```

2. 分别创建子表 avoid_hot.user_info_201905 和 avoid_hot.user_info_201906
   
   ```lang-javascript
   > db.avoid_hot.createCL( "user_info_201905", { ShardingKey: { id: 1 }, ShardingType: "hash", AutoSplit: true } )
   > db.avoid_hot.createCL( "user_info_201906", { ShardingKey: { id: 1 }, ShardingType: "hash", AutoSplit: true } )
   ```

3. 分别将子表挂载到主表上

   ```lang-javascript 
   > db.avoid_hot.user_info.attachCL( "avoid_hot.user_info_201905", { LowBound: { date: "2019-05-01" }, UpBound: { date: "2019-06-01" } } )
   > db.avoid_hot.user_info.attachCL( "avoid_hot.user_info_201906", { LowBound: { date: "2019-06-01" }, UpBound: { date: "2019-07-01" } } )
   ```

### 某个数据组繁忙监测 ###

单个数据组繁忙程度远高于其它数据组时，该数据节点对机器资源的消耗远高于其它数据节点，用户可以通过 CPU 性能、磁盘性能和快照的方式进行监测。

- **CPU 性能监测**

   使用 `top` 监测各进程占用机器资源的多少，当发现某 SequoiaDB 节点占用 CPU 资源远高于其他节点占用的 CPU 资源时，表示有热点问题

   ```lang-bash
   $ top
   Tasks: 400 total,   1 running, 399 sleeping,   0 stopped,   0 zombie
   %Cpu(s):  81.0 us,  2.7 sy,  0.0 ni, 95.3 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
   KiB Mem:   2902952 total,  2880400 used,    22552 free,        0 buffers
   KiB Swap:  2097148 total,    11772 used,  2085376 free.  1831100 cached Mem
   
      PID USER      PR  NI    VIRT    RES    SHR S %CPU %MEM     TIME+ COMMAND                  
     4974 sdbadmin  20   0 2082952 122816  21464 S 58.3  4.2   0:06.91 sequoiadb(11840) D   
     4973 sdbadmin  20   0  738828  67332  12896 S 20.3  2.3   0:00.88 sequoiadb(11810) S                                
     4980 sdbadmin  20   0 1848188 111692  17936 S  0.3  3.8   0:01.03 sdbom(11780) 
     4985 sdbadmin  20   0 1689736  85572  13212 S  0.3  2.9   0:04.44 sequoiadb(11830) D
     4988 sdbadmin  20   0 2400668  92708  14180 S  0.3  3.2   0:01.20 sequoiadb(11800) C    
   ```

- **磁盘性能监测**

   使用 `iostat` 监控磁盘工作状况，如果在多个刷盘周期内，其中一块磁盘表现活跃，而其他磁盘几乎无刷盘行为，则热点问题出现在较活跃磁盘上的数据中。

   ```lang-bash
   $ iostat -x -k 5
   Linux 3.10.0-123.el7.x86_64 (test) 	06/03/2019 	_x86_64_	(1 CPU)
   
   avg-cpu:  %user   %nice %system %iowait  %steal   %idle
              5.55    0.00   10.69    3.52    0.00   80.24
   
   Device:         rrqm/s   wrqm/s     r/s     w/s    rkB/s    wkB/s avgrq-sz avgqu-sz   await r_await w_await  svctm  %util
   sda               1.67     4.26   48.63    3.14  4445.49   396.88   187.09     0.46    8.92    5.07   68.42   1.65   8.52
   scd0              0.00     0.00    0.02    0.00     0.07     0.00     8.00     0.00    1.09    1.09    0.00   1.09   0.00
   dm-0              0.00     0.00   47.91    2.72  4398.06   375.82   188.58     0.47    9.22    5.26   79.11   1.67   8.45
   dm-1              0.00     0.00    0.87    4.48     3.85    17.91     8.14     0.37   69.01    0.58   82.28   1.14   0.61
   ```

- **snapshot 监测**

   用户确定到某个数据节点存在热点问题后，可以通过 [snapshot()][db_snapshot] 接口查找当前正在执行的操作，确定问题表。

   1. 抓取快照信息

     ```lang-javascript
     > db.snapshot(SDB_SNAP_SESSIONS,{NodeName:"sdbserver1:11840"})
     ```

   2. 查找当前正在执行的操作

     ```lang-bash
     {
       "NodeName": "sdbserver1:11840",
       "SessionID": 17554,
       "TID": 31317,
       "Status": "Waiting",
       "Type": "ShardAgent",
       "Name": "Type:Shard,NetID:10,R-TID:27626,R-IP:192.168.1.80,R-Port:11810",
       "Source": "MySQL-2",
       "QueueSize": 0,
       "ProcessEventCount": 18447,
       "RelatedID": "c0a801502e220000000000004492",
       "Contexts": [],
       "TotalDataRead": 1983,
       "TotalIndexRead": 14789,
       "TotalDataWrite": 1404,
       "TotalIndexWrite": 5757,
       "TotalUpdate": 456,
       "TotalDelete": 0,
       "TotalInsert": 948,
       "TotalSelect": 1527,
       "TotalRead": 1983,
       "TotalReadTime": 0,
       "TotalWriteTime": 0,
       "ReadTimeSpent": 0,
       "WriteTimeSpent": 0,
       "ConnectTimestamp": "2019-05-07-08.08.26.265864",
       "ResetTimestamp": "2019-05-07-08.08.26.265864",
       "LastOpType": "QUERY",
       "LastOpBegin": "--",
       "LastOpEnd": "2019-05-07-09.14.39.716133",
       "LastOpInfo": "Collection:avoid_hot.user_operand, Matcher:{ \"$and\": [ { \"$and\": [ { \"NAME_\": { \"$et\": \"applid\" } }, { \"EXECUTION_ID_\": { \"$et\": \"7b36d6e2-7065-11e9-a807-0050569f53de\" } }, { \"TASK_ID_\": { \"$isnull\": 1 } } ] }, { \"TASK_ID_\": { \"$isnull\": 1 } } ] }, Selector:{}, OrderBy:{ \"TASK_ID_\": 1 }, Hint:{ \"\": \"ACT_IDX_VARIABLE_TASK_ID\" }, Skip:0, Limit:-1, Flag:0x00000200(512)",
       "UserCPU": 5.52,
       "SysCPU": 3.34
     }
     ```

   > **Note:**
   >
   > 在 SDB_SNAP_SESSIONS 快照中，LastOpInfo 项是该线程正在执行的操作 SQL，其中的 Collection:avoid_hot.user_operand 是集合名称。


主数据节点繁忙
----

主数据节点繁忙程度远高于同组内备数据节点时，代表当前大量的数据操作为查询操作，可以将部分查询操作分散到备数据节点。

设置备数据节点承担部分查询操作，需要将 --preferredinstance 配置为 A，保证主备节点繁忙度基本相同

```lang-bash
> db.updateConf( { preferredinstance: "A" } )
```

### 主数据节点繁忙的监测 ###

主数据节点繁忙程度远高于备节点的监测，主要通过监控 Linux 系统 CPU 利用率来实现，对比同数据组内不同数据节点的 CPU 利用率，即可判断出是否存在繁忙程度差异。

- 11840 主节点 CPU 占用率

   ```lang-bash
   $ top
   Tasks: 400 total,   1 running, 399 sleeping,   0 stopped,   0 zombie
   %Cpu(s):  81.0 us,  2.7 sy,  0.0 ni, 95.3 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
   KiB Mem:   2902952 total,  2880400 used,    22552 free,        0 buffers
   KiB Swap:  2097148 total,    11772 used,  2085376 free.  1831100 cached Mem
   
      PID USER      PR  NI    VIRT    RES    SHR S %CPU %MEM     TIME+ COMMAND
     4974 sdbadmin  20   0 2082952 122816  21464 S 58.3  4.2   0:06.91 sequoiadb(11840) D   
     4973 sdbadmin  20   0  738828  67332  12896 S 20.3  2.3   0:00.88 sequoiadb(11810) S
     4980 sdbadmin  20   0 1848188 111692  17936 S  0.3  3.8   0:01.03 sdbom(11780)   
     4985 sdbadmin  20   0 1689736  85572  13212 S  0.3  2.9   0:04.44 sequoiadb(11830) D  
     4988 sdbadmin  20   0 2400668  92708  14180 S  0.3  3.2   0:01.20 sequoiadb(11800) C   
   ```

- 11840 备节点 CPU 占用率

   ```lang-bash
   $ top
   top - 02:28:45 up  2:46,  2 users,  load average: 0.18, 0.23, 0.35
   Tasks: 400 total,   1 running, 399 sleeping,   0 stopped,   0 zombie
   %Cpu(s):  8.1 us,  9.1 sy,  0.0 ni, 82.5 id,  0.0 wa,  0.0 hi,  0.3 si,  0.0 st
   KiB Mem:   2902952 total,  2876196 used,    26756 free,        0 buffers
   KiB Swap:  2097148 total,    11700 used,  2085448 free.  1807664 cached Mem
   
      PID USER      PR  NI    VIRT    RES    SHR S %CPU %MEM     TIME+ COMMAND         
     4974 sdbadmin  20   0 2082952 122828  21464 S  0.7  4.2   0:29.79 sequoiadb(11840) D  
     4980 sdbadmin  20   0 1848188 111728  17936 S  0.7  3.8   0:21.99 sdbom(11780) 
     4712 sdbadmin  20   0  632860  10052   6516 S  0.3  0.3   0:11.79 sdbcm(11790) 
     4973 sdbadmin  20   0  828984  69464  13700 S  0.3  2.4   0:20.66 sequoiadb(11810) S
     4985 sdbadmin  20   0 1689736  85584  13212 S  0.3  2.9   0:26.32 sequoiadb(11830) D
     4988 sdbadmin  20   0 2400668  92708  14180 S  0.3  3.2   0:31.63 sequoiadb(11800) C 
     4991 sdbadmin  20   0 1689736 115928  21460 S  0.3  4.0   0:29.18 sequoiadb(11820) D 
   ```

> **Note:**
>
>- 11840 主节点的 CPU 利用率较高，备节点几乎不消耗 CPU 资源，所以 11840 主节点繁忙程度更高，可以通过配置读写分离来优化数据库性能。
>- CPU 利用率的比较主要是主节点间 CPU 利用率的比较、同机器不同节点间 CPU 利用率的比较以及同组内不同节点间 CPU 利用率的比较

其他情况下的读写热点监控，同样可以通过该方法进行。

[^_^]:
    本文使用到的所有链接
    
[sharding_key]:manual/Distributed_Engine/Architecture/Sharding/sharding_keys.md
[multi_dimension]:manual/Distributed_Engine/Architecture/Sharding/multi_dimension.md
[db_snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
