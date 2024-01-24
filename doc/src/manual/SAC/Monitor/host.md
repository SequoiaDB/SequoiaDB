
主机列表
----
通过主机列表，用户可以查看安装当前服务的所有主机的配置信息，包括主机 IP、CPU 型号、内存大小、磁盘容量、操作系统和网卡数。

![主机列表][hosts_list_1]

> **Note:**
>
> - 需要了解指定主机的详细信息时，用户可以点击列表中的主机名进入[主机信息][information]页。
> - 需要排序时，用户可以通过点击列表表头来根据字段进行排序。
> - 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。

图表
----
主机图表页面支持查看当前服务所有主机的磁盘利用率、CPU 利用率、网络流量和内存利用率在近 30s 内的实时信息。

![图表][hosts_list_chart]


主机快照
----
主机快照页面支持查看已安装当前服务的主机的状态信息。用户可以自行选择需要显示在列表中的信息。同时，主机快照页面支持筛选搜索功能和实时刷新功能。

![主机快照][host_snapshot_1]

该页面显示安装了当前服务的主机的主机名、IP、CPU 使用率、内存占用量、磁盘占用量和网卡收发流量。需要了解指定主机的详细信息时，用户可以点击列表中的主机名进入[主机信息][information]页面。

>**Note:**
>
> - 需要选择显示哪些字段，用户可以点击列表上方的 **选择显示列** 按钮来进行选择。
> - 需要排序时，用户可以点击列表表头来根据字段进行排序。
> - 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。
> 主机快照对应的字段说明，可通过[操作系统快照][SDB_SNAP_SYSTEM]查看。


用户可以通过点击 **实时刷新设置** 按钮开启实时刷新后，如果列表中的数据相比前一次获取时有上升，那么该数据字体颜色将为红色；如果数据下降，字体颜色则为绿色。

![主机快照实时刷新][host_snapshot_2]

> **Note:**
>

## 主机信息

主机信息页面可以查看当前主机的的配置信息、CPU 利用率、内存利用率、网络流量、磁盘利用率和挂载在该主机的服务信息。
![主机信息][host_info_1]

主机信息有五个子页面，分别是：
- **CPU 页面**

   可以查看当前所选主机的 CPU 信息，包括主频、内核数、逻辑处理器、三级缓存和 CPU 实时利用率
![CPU][host_info_cpu]

- **内存页面**

   可以查看所选主机的内存使用情况
![内存][host_info_memory]

- **磁盘页面**

   可以查看当前所选主机的所有磁盘信息，包括磁盘利用率、磁盘读写 IO、容量等
![磁盘][host_info_disk]

- **网卡页面**
  
   可以查看所选主机的所有网卡信息，包括收发流量、收发数据包和实时收发流量速率
![网卡][host_info_net]

- **图表页面**

   可以查看所选主机的磁盘利用率、CPU 利用率、网络流量和内存利用率的实时信息
![图表][host_info_charts]


[^_^]:
    本文使用的所有引用及链接
[SDB_SNAP_SYSTEM]:manual/Manual/Snapshot/SDB_SNAP_SYSTEM.md

[information]:manual/SAC/Monitor/host.md#主机信息
[hosts_list_1]:images/SAC/Monitor/hosts_list_1.png
[hosts_list_chart]:images/SAC/Monitor/hosts_list_chart.png
[host_snapshot_1]:images/SAC/Monitor/host_snapshot_1.png
[host_snapshot_2]:images/SAC/Monitor/host_snapshot_2.png
[host_info_1]:images/SAC/Monitor/host_info_1.png
[host_info_cpu]:images/SAC/Monitor/host_info_cpu.png
[host_info_memory]:images/SAC/Monitor/host_info_memory.png
[host_info_disk]:images/SAC/Monitor/host_info_disk.png
[host_info_net]:images/SAC/Monitor/host_info_net.png
[host_info_charts]:images/SAC/Monitor/host_info_charts.png
