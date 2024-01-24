[^_^]:
    分区性能监控
    作者：魏彰凯
    时间：20190531
    评审意见
    王涛：
    许建辉：
    市场部：20190819
       

本文介绍在 SequoiaDB 巨杉数据库中，如何对[数据分区][sharding]进行性能监控，比较全面地了解 SequoiaDB 的运行状况。

数据量的均衡
----

在 SequoiaDB 中，通过[分区键][sharding_key]将数据打散到不同的数据分区中，不同数据分区内数据量的大小，将直接影响到数据分区的性能。不同分区键数据量的均衡，可以有效提高数据库的整体性能。不同分区数据量的多少可以通过查看不同节点下的数据文件大小进行判断，在不同数据节点下，数据文件大小差距过大，则判断为数据失衡。
通常情况下，SequoiaDB 每个数据节点对应一块磁盘，可以通过 `df` 命令查看磁盘使用情况，确定数据量是否均衡。

```lang-bash
# df -h
Filesystem             Size  Used Avail Use% Mounted on
/dev/mapper/rhel-root   50G   42G  8.3G  84% /
devtmpfs                32G     0   32G   0% /dev
tmpfs                   32G   84K   32G   1% /dev/shm
tmpfs                   32G  450M   32G   2% /run
tmpfs                   32G     0   32G   0% /sys/fs/cgroup
/dev/mapper/rhel-home   42G   22G   21G  52% /home
/dev/sda               497M  140M  358M  29% /boot
/dev/sdb               1.2T  140M  1.2T   1% /sdbdata/disk1
/dev/sdc               1.2T  140M  1.2T   1% /sdbdata/disk2
/dev/sdd               1.2T  140M  1.2T   1% /sdbdata/disk3
/dev/sde               1.2T  140M  1.2T   1% /sdbdata/disk4
tmpfs                  6.3G   16K  6.3G   1% /run/user/42
tmpfs                  6.3G     0  6.3G   0% /run/user/0
tmpfs                  6.3G     0  6.3G   0% /run/user/1001
```

> **Note:**
> 
> 主要关注磁盘的使用率和不同磁盘间数据量的差异。磁盘使用率超过 70%，需要规划扩容相关内容。磁盘极限占用率推荐为 80%，预留一定的存储空间。如果发现 SequoiaDB 数据节点所在的磁盘间数据量差距过大，则需要通过[分区数据均衡][sharding_balance]来定位并解决问题。

IO性能的均衡
----

通过 `iostat` 命令监控 IO 性能， 重点观察 %iowait 是否超过 5%，及各磁盘读写速度是否相近

```lang-bash
# iostat -xm 5
Linux 3.10.0-123.el7.x86_64 (test) 	06/05/2019 	_x86_64_	(1 CPU)

avg-cpu:  %user   %nice %system %iowait  %steal   %idle
           0.62    0.00    1.03    0.00    0.00   98.35

Device:         rrqm/s   wrqm/s     r/s     w/s    rMB/s    wMB/s avgrq-sz avgqu-sz   await r_await w_await  svctm  %util
scd0              0.00     0.00    0.00    0.00     0.00     0.00     0.00     0.00    0.00    0.00    0.00   0.00   0.00
sda               0.00     0.00    0.21    0.00     0.00     0.00    16.00     0.00    1.00    1.00    0.00   1.00   0.02
dm-0              0.00     0.00    0.21    0.00     0.00     0.00    16.00     0.00    1.00    1.00    0.00   1.00   0.02
dm-1              0.00     0.00    0.00    0.00     0.00     0.00     0.00     0.00    0.00    0.00    0.00   0.00   0.00
```

CPU使用率监测
----

通过 `top` 命令监控 CPU 使用率，重点观察 CPU 使用率是否可以达到较高水平，但是 sys% 不应该超过 usr% 的 50%

```lang-bash
# top -c
top - 01:37:33 up  1:43,  4 users,  load average: 0.02, 0.10, 0.12
Tasks: 406 total,   1 running, 405 sleeping,   0 stopped,   0 zombie
%Cpu(s):  1.4 us,  1.7 sy,  0.0 ni, 97.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
KiB Mem:   2902952 total,  2882112 used,    20840 free,        4 buffers
KiB Swap:  2097148 total,     6664 used,  2090484 free.  1890884 cached Mem

   PID USER      PR  NI    VIRT    RES    SHR S %CPU %MEM     TIME+ COMMAND                                                                                                                                      
  7475 sdbadmin  20   0  123892   2008   1236 R  1.3  0.1   0:00.15 top -c 
  4534 sdbadmin  20   0 1706116  87136  13544 S  0.7  3.0   0:22.02 sequoiadb(11830) D 
  4537 sdbadmin  20   0 1848192 111804  18016 S  0.7  3.9   0:14.70 sdbom(11780)
  4545 sdbadmin  20   0  837180  85492  13836 S  0.7  2.9   0:22.63 sequoiadb(11810) S 
  4551 sdbadmin  20   0 2490824  93316  14608 S  0.7  3.2   0:25.89 sequoiadb(11800) C
  4542 sdbadmin  20   0 1706116 116440  21784 S  0.3  4.0   0:23.83 sequoiadb(11820) D
  4548 sdbadmin  20   0 2099332 124368  21788 S  0.3  4.3   0:24.33 sequoiadb(11840) D
  ...
```

分区健康检查
----

当磁盘 I/O 和 CPU 使用率均正常，但 SequoiaDB 的数据分区依然无法正常提供服务时，需要进行分区健康检查

```lang-bash
> db.snapshot( SDB_SNAP_HEALTH, { GroupName: "group1" } )
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver1:11820",
  "IsPrimary": true,
  "ServiceStatus": true,
  "Status": "Normal",
  "BeginLSN": {
    "Offset": 1342177280,
    "Version": 6
  },
  "CurrentLSN": {
    "Offset": 2657251884,
    "Version": 9
  },
  "CommittedLSN": {
    "Offset": 2657251884,
    "Version": 9
  },
  "CompleteLSN": 2657251968,
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
    "OpenFiles": 60000,
    "NumProc": 257587,
    "FileSize": -1
  },
  "ResetTimestamp": "2019-05-29-17.04.24.775434",
  "ErrNum": {
    "SDB_OOM": 0,
    "SDB_NOSPC": 0,
    "SDB_TOO_MANY_OPEN_FD": 0
  },
  "Memory": {
    "LoadPercent": 2,
    "TotalRAM": 67556651008,
    "RssSize": 1784561664,
    "LoadPercentVM": 45,
    "VMLimit": 65878863872,
    "VMSize": 29887328256
  },
  "Disk": {
    "Name": "/dev/mapper/rhel-root",
    "LoadPercent": 83,
    "TotalSpace": 53660876800,
    "FreeSpace": 8917061632
  },
  "FileDesp": {
    "LoadPercent": 0,
    "TotalNum": 60000,
    "FreeNum": 59921
  },
  "StartHistory": [
    "2019-05-29-17.04.25.211680",
    "2019-05-27-11.28.00.190238",
    "2019-04-27-22.04.03.000814",
    "2019-04-27-22.00.23.301800"
  ],
  "AbnormalHistory": [],
  "DiffLSNWithPrimary": 0
}
{
  "NodeName": "sdbserver1:11820",
  "IsPrimary": false,
  "ServiceStatus": true,
  "Status": "Normal",
  "BeginLSN": {
    "Offset": 1342177280,
    "Version": 6
  },
  "CurrentLSN": {
    "Offset": 2657251884,
    "Version": 9
  },
  "CommittedLSN": {
    "Offset": 2657251884,
    "Version": 9
  },
  "CompleteLSN": 2657251968,
  "LSNQueSize": 0,
  "NodeID": [
    1000,
    1001
  ],
  "DataStatus": "Normal",
  "SyncControl": false,
  "Ulimit": {
    "CoreFileSize": 0,
    "VirtualMemory": -1,
    "OpenFiles": 60000,
    "NumProc": 128578,
    "FileSize": -1
  },
  "ResetTimestamp": "2019-05-29-17.04.06.253067",
  "ErrNum": {
    "SDB_OOM": 0,
    "SDB_NOSPC": 0,
    "SDB_TOO_MANY_OPEN_FD": 0
  },
  "Memory": {
    "LoadPercent": 3,
    "TotalRAM": 33737936896,
    "RssSize": 1341104128,
    "LoadPercentVM": 34,
    "VMLimit": 37132955648,
    "VMSize": 12665397248
  },
  "Disk": {
    "Name": "/dev/mapper/rhel-root",
    "LoadPercent": 75,
    "TotalSpace": 53660876800,
    "FreeSpace": 13171359744
  },
  "FileDesp": {
    "LoadPercent": 0,
    "TotalNum": 60000,
    "FreeNum": 59921
  },
  "StartHistory": [
    "2019-05-29-17.04.06.313152",
    "2019-05-27-11.26.27.818497",
    "2019-04-27-22.03.50.506024",
    "2019-04-27-22.00.25.508364"
  ],
  "AbnormalHistory": [],
  "DiffLSNWithPrimary": 0
}
```
> **Note:**
> 
> 重点关注分区内每一个节点的 ServiceStatus 是否为 true

分区内节点LSN的一致性
----

在 SequoiaDB 中，主节点是复制组内唯一可以进行写操作的成员。当发生写操作时，主节点写入数据，并记录事务日志 `replicalog`。备节点从主节点异步复制 `replicalog`，并通过重放 `replicalog` 来复制数据。主备节点间的同步，使用 LSN 号标识数据的顺序。

主备节点的 LSN 需要尽量保持一致，或者差距很小，才能保证数据库的正常运行。主备节点间 LSN 差距过大，会触发节点间的全量同步。

分区内节点 LSN 的一致性可以通过 [sdbinspect][consistency_check] 工具进行检测。

查看当前集群所有节点 LSN 版本一致性

```lang-bash
# sdbinspect -d localhost:11810 -o item.bin
```

输出结果如下：

```lang-text
inspect done
Inspect result:
Total inspected group count       : 3
Total inspected collection        : 15
Total different collections count : 0
Total different records count     : 0
Total time cost                   : 32904 ms
Reason for exit : exit with no records different
```

[^_^]:
    本文使用到的所有链接
    
[sharding]:manual/Distributed_Engine/Architecture/Sharding/Readme.md
[sharding_key]:manual/Distributed_Engine/Architecture/Sharding/sharding_keys.md
[consistency_check]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/consistency_check.md
[sharding_balance]:manual/Distributed_Engine/Maintainance/Sharding_Management/balance.md