此部分是 [PHP API](api/php/html/index.html) 相关文档。

##历史更新情况##

**Version 2.10**

SequoiaCL 类添加接口

 * enableSharding：对集合启用分区功能
 * disableSharding：对集合关闭分区功能
 * enableCompression：对集合启用压缩功能
 * disableCompression：对集合关闭压缩功能
 * setAttributes：对集合的属性进行修改

SequoiaCS 类添加接口

 * alterCollectionSpace：对集合空间的属性进行修改
 * setAttributes：对集合空间的属性进行修改
 * setDomain：修改集合空间所属的域
 * removeDomain：移除集合空间所属的域

SequoiaDomain 类添加接口

 * addGroups：向域中添加数据组
 * setGroups：对域设置数据组
 * removeGroups：移除属于域的某些数据组
 * setAttributes：设置域的属性

**Version 2.9**

SequoiaDB 类添加接口

* analyze：分析集合和索引的数据，并收集统计信息

**Version 2.8.5**

SequoiaDB 类添加接口

* getSessionAttr：获取会话设置属性

**Version 2.8**

SequoiaDB 类添加接口

* syncDB：持久化数据和日志到磁盘
* forceSession：终止指定会话的当前操作

