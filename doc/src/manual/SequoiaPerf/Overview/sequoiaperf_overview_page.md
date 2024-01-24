[^_^]:
    目录名称：SequoiaPerf概览

SequoiaPerf 概览包括集群主要概览页面和导航页面。

##集群主要概览##

集群主要概览页面会显示 SQL 集群、SequoiaDB 集群以及集群主机的部分监控信息，该页面包含如下内容：

![SequoiaPerf集群主要概览][sequoiaperf_overview]

- 集群中最早启动 SQL 实例运行时间
- 集群中最近启动 SQL 实例运行时间
- SQL 实例状态
- 最慢的 SQL 慢查询时间
- SQL 慢查询数量
- 集群中最早启动 SDB 节点运行时间
- 集群中最近启动 SDB 节点运行时间
- SDB 节点状态
- 最慢的 SDB 慢查询时间
- SDB 慢查询数量
- 服务器 IP、主机名、CPU、内存、磁盘使用情况

> **Note:**
>
> 页面左上部分为集群实例名称，点击该名称可以显示导航页面；页面右上部分为所显示数据的时间范围区间及页面数据刷新频率，点击对应值可以更改对应参数。

##导航##

导航页面包含 SequoiaPerf 所有的页面链接，用户可以通过点击链接访问相应的页面。

  ![导航][sequoiaperf_navigation]


[^_^]:
    本文使用的所有引用及链接

[deployment]:manual/SequoiaPerf/Deployment/sequoiaperf_deployment.md
[management]:manual/SequoiaPerf/Deployment/sequoiaperf_management.md
[add_sql_instance]:manual/SequoiaPerf/Deployment/sequoiaperf_add_sql.md
[sequoiaperf_overview]:images/SequoiaPerf/Overview/sequoiaperf_overview.png
[sequoiaperf_navigation]:images/SequoiaPerf/Overview/sequoiaperf_navigation.png


