[^_^]:
    SQL 慢查询监控详情

SQL 慢查询监控详情可以通过 SQL 查询细节页面查看。该页面包含如下内容：

![SQL 查询细节页面][sequoiaperf_sql_slowquery_detail_image]

- Query Time Spent：SQL 查询语句的耗时
- Start Time：查询开始时间
- Schema：Schema 名称
- Query Text：SQL 语句
- Related SDB Operation：[相关 SequoiaDB 操作详情][sequoiaperf_sdb_slowquery_detail]
- SQL Client：SQL 客户端
- Thread ID：SQL 实例处理查询的线程 ID
- LockWaitTime：SQL 线程等待获取锁的时间




[^_^]:
    本文使用的所有引用及链接
[sequoiaperf_sdb_slowquery_detail]:manual/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sdb_slowquery_detail_page.md
[sequoiaperf_sql_slowquery_detail_image]:images/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sql_slowquery_detail.png

