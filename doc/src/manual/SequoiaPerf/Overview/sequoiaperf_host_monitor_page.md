[^_^]:
    目录名称：主机概览

用户完成[添加 sequoiaperf_node_exporter 服务][add_agent_service]后，可以在主机概览页面查看相关监控信息，该页面包含如下内容：

![主机概览][sequoiaperf_host_overview]

- 概览信息
  - 主机运行时间
  - 总内存、内存使用率
  - 磁盘使用情况
  - 每秒网络带宽
  - CPU 使用率

- CPU
  - CPU 核数
  - CPU iowait
  - 系统平均负载

- 磁盘
  - 磁盘使用率
  - 每1秒内 I/O 操作耗时占比
  - 磁盘读写速率（IOPS）
  - 每秒磁盘读写容量
  - 每秒 I/O 读写的耗时

- 内存
  - 内存使用情况

- 网络
  - 网络 Socket 连接信息
  - 每小时网络流量

> **Note:**
>
> - 页面左上部分可以选择主机和网卡，获取指定主机或网卡的信息。
> - 除概览信息外，其余监控面板默认不展开。


[^_^]:
    本文使用的所有引用及链接
[add_agent_service]:manual/SequoiaPerf/Configuration/sequoiaperf_add_agent.md
[sequoiaperf_host_overview]:images/SequoiaPerf/Overview/sequoiaperf_host_overview.png

