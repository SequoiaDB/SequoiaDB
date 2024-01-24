[^_^]:
    目录名称：MySQL概览

用户完成[添加 SQL 实例][add_sql_instance]后，可以在 MySQL 概览页面查看相关监控信息，该页面包含如下内容：

![MySQL概览][sequoiaperf_mysql_overview]

- 概览信息
  - MySQL Uptime：MySQL 实例正常运行时间
  - Current QPS：当前每秒查询数
  - Current TPS：当前每秒事务数
  - Bytes Sent Rate：发送字节数
  - Bytes Received Rate：字节接收率

- Connections
  - MySQL Max Connections：MySQL 连接数量
  - MySQL Client Thread Activity：MySQL 客户端线程数量

- SQL
  - Top Frequency SQL：高频 SQL 语句列表
  - Average User SQL Running Time：用户 SQL 语句执行平均时间

- Commands
  - Most Used Commands：MySQL 最常用命令
  - Hourly Most Used Commands：MySQL 每小时最常用命令

- Network
  - MySQL Network Traffic：MySQL 网络进出流量 
  - MySQL Network Hourly Usage：MySQL 每小时网络进出流量

- Process Stats
  - Process States：MySQL 进程状态
  - Top Process States Hourly：MySQL 每小时内最常用进程状态

- Others
  - MySQL Table Definition Cache：MySQL 表定义使用缓存状况



> **Note:**
>
> 页面左上部分可以选择 MySQL 实例组内的 MySQL 实例，获取指定实例的信息。如果添加 MySQL 实例时没有指定实例组，那么实例组将显示为 Null。



[^_^]:
    本文使用的所有引用及链接
[add_sql_instance]:manual/SequoiaPerf/Configuration/sequoiaperf_add_sql.md
[sequoiaperf_mysql_overview]:images/SequoiaPerf/Overview/sequoiaperf_mysql_overview.png

