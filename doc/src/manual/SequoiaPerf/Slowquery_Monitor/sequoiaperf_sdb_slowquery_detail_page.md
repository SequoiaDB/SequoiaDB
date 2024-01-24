[^_^]:
     目录名称：SequoiaDB慢查询监控详情

SequoiaDB 慢查询监控详情，可以通过 SequoiaDB 慢查语句细节页面查看。该页面包含如下内容：

- Operation Time Spent：查询已经耗时
- Operation Start Time：查询开始时间
- Operation End Time：查询结束时间
- Collection Space：所查询的集合空间
- Collection：所查询的集合
- Operation Type：操作类型
- Query Text：SDB 查询语句内容
- Related SQL Statement：[对应的 SQL 端慢查询内容页面链接][sequoiaperf_sql_slowquery_detail]
- Node ID：节点 ID
- Node Name：节点名
- Host Name：主机名
- Group ID：组 ID
- Group Name：组名
- TID：线程 ID
- Related Node IDs：经该协调节点发送到的远程节点 ID 集合
- Related Node Names：远程节点名集合
- Remote Node Wait Time：等待远程节点处理时间
- Records Returned：返回记录数
- Total Message Sent：发送消息总量
- Total Message Sent Time：发送消息总时间
- Active Operation：查询是否还在运行
- [相应数据节点的慢查询详情页面连接][sequoiaperf_sdb_slowquery_datanode_detail]

![SDB慢查询语句细节页面][sequoiaperf_sdb_slowquery_detail]




[^_^]:
    本文使用的所有引用及链接
[sequoiaperf_sdb_slowquery_detail]: images/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sdb_slowquery_detail.png
[sequoiaperf_sdb_slowquery_datanode_detail]:manual/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sdb_slowquery_datanode_detail_page.md
[sequoiaperf_sql_slowquery_detail]:manual/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sql_slowquery_detail_page.md


