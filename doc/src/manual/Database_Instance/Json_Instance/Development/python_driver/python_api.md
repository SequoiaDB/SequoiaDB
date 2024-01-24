此部分是 [Python API](api/python/html/index.html) 相关文档。

##历史更新情况##

**Version 2.10**

collection 类添加接口

 * alter：对集合的属性进行修改
 * enable_sharding：对集合启用分区功能
 * disable_sharding：对集合关闭分区功能
 * enable_compression：对集合启用压缩功能
 * disable_compression：对集合关闭压缩功能
 * set_attributes：对集合的属性进行修改

collectionspace 类添加接口

 * alter：对集合空间的属性进行修改
 * set_attributes：对集合空间的属性进行修改
 * set_domain：修改集合空间所属的域
 * remove_domain：移除集合空间所属的域

domain 类添加接口

 * add_groups：向域中添加数据组
 * set_groups：对域设置数据组
 * remove_groups：移除属于域的某些数据组
 * set_attributes：设置域的属性

**Version 2.9**

client 添加接口

- analyze：分析集合和索引的数据，并收集统计信息

**Version 2.8.5**

client 添加接口

- get_session_attri：获取会话设置属性

**Version 2.8**

client 添加接口

 * list_domains：查看域列表
 * is_domain_existing：查看域是否存在
 * create_domain：创建域
 * drop_domain：删除域
 * get_domain：获取域对象
 * get_cata_replica_group：获取编目节点组对象
 * get_coord_replica_group：获取协调节点组对象
 * create_cata_replica_group：创建编目节点组
 * create_coord_replica_group：创建协调节点组
 * remove_cata_replica_group：删除编目节点组
 * remove_coord_replica_group：删除协调节点组
 * start_replica_group：启动复制组
 * stop_replica_group：停止复制组

增加接口类 domain 和相关接口

 * alter：修改域属性
 * list_collection_spaces：查看属于该域的集合空间列表
 * list_collections：查看属于该域的集合列表

**Version 1.10**

增加接口类 lob 和相关接口

 * close：关闭创建的 lob 对象，用以刷新数据
 * read：可从 lob 对象中读取数据
 * write：可把数据写入 lob
 * seek：可跳转到到指定数据位置
 * get_oid：可获取 lob 对象的 oid
 * get_size：可获取 lob 对象的大小(bytes)
 * get_create_time：可获取 lob 对象的创建时间

collection 添加接口

 * create_lob：可在当前的 collection 中创建一个 lob 对象
 * remove_lob：可在当前的 collection 中删除指定 lob 对象
 * get_lob：可获取当前 collection 中指定 oid 的 lob 对象
 * list_lobs：可列出当前 collection 中所有的 lob

