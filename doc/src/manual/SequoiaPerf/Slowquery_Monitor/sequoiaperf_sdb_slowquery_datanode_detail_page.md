[^_^]:
     目录名称：SequoiaDB慢查询对应的数据节点详情

SequoiaDB 慢查询对应的数据节点详情，可以通过 SequoiaDB 慢查语句数据节点细节页面查看。该页面包含如下内容：

- Operation Time Spent：查询已经耗时
- Operation Start Time：查询开始时间
- Operation End Time：查询结束时间
- Collection Space：所查询的集合空间
- Collection：所查询的集合
- Operation Type：操作类型
- Query Text：SDB 查询语句内容
- Node ID：节点 ID
- Node Name：节点名
- Host Name：主机名
- Group ID：组 ID
- Group Name：组名
- TID：线程 ID
- Session ID：会话 ID
- Data Read：数据读
- Data Write：数据写
- Index Read：索引读
- Index Write：索引写
- Records Returned：返回记录数
- LOB Read：LOB 读
- LOB Write：LOB 写
- Total Message Sent Time：发送消息总时间
- TransLock Wait Time：锁等待时间
- Latch Wait Time：闩锁等待时间
- [相应访问计划细节页面连接][sequoiaperf_sdb_slowquery_accessplan]

![SequoiaDB 慢查询语句数据节点细节页面][sequoiaperf_sdb_slowquery_datanode_detail]



[^_^]:
    本文使用的所有引用及链接
[sequoiaperf_sdb_slowquery_datanode_detail]: images/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sdb_slowquery_datanode_detail.png
[sequoiaperf_sdb_slowquery_accessplan]: manual/SequoiaPerf/Slowquery_Monitor/sequoiaperf_sdb_slowquery_accessplan_page.md 


