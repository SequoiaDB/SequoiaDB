[^_^]:
    目录名：SequoiaPerf配置

##SequoiaPerf配置##

该页面可以对集群进行配置，如添加 SQL 实例数据导出器、创建快照等。同时，支持配置集群在 SDB 和 SQL 导出器配置基础上的新慢查询过滤阈值等。

![SequoiaPerf配置][sequoiaperf_configuration]

##SequoiaDB配置##

该页面可以配置 SequoiaDB 数据库的慢查询阈值、快照存储的记录数等。

![SDB配置][sdb_configuration]

##SQL配置##

该页面可以配置所添加的 SQL 实例导出器的慢查询阈值、信息抓取频率等。

![SQL配置][sql_configuration]

##告警配置##

该页面可以配置告警项 Info、Warning、Error 和 Critical 四个级别的告警阈值。

实际值达到或高于设置的值就会触发报警。如果实际值满足多个告警等级，实际只报最高等级的警。

如果告警阈值间没有升序的大小关系，可能会产生多条告警。

![告警配置][alert_configuration]




[^_^]:
    本文使用的所有引用及链接
[sequoiaperf_configuration]:images/SequoiaPerf/Configuration/sequoiaperf_configuration.png
[sdb_configuration]:images/SequoiaPerf/Configuration/sequoiaperf_sdb_configuration.png
[sql_configuration]:images/SequoiaPerf/Configuration/sequoiaperf_sql_configuration.png
[alert_configuration]:images/SequoiaPerf/Configuration/sequoiaperf_alert_configuration.png

