此部分是 [CSharp API](api/cs/html/index.html) 相关文档。

> **Note:** 
>
> * 删除接口 - 不再兼容 
> * 废弃接口 - 保持兼容性

##历史更新情况##

**Version 2.10**

SequoiaDB::DBCollection 添加接口

 * OpenLob(ObjectId id, int mode)：以指定模式打开 lob，其中 mode 取值为 DBLob.SDB_LOB_READ 或 DBLob.SDB_LOB_WRITE
 * enableSharding：对集合启用分区功能
 * disableSharding：对集合关闭分区功能
 * enableCompression：对集合启用压缩功能
 * disableCompression：对集合关闭压缩功能
 * setAttributes：对集合的属性进行修改

SequoiaDB::DBLob 添加接口

 * Lock：锁定 lob 以进行写入 
 * LockAndSeek：锁定 lob 以进行写入的同时查找偏移位置
 * GetModificationTime：获取 lob 最后修改时间
 * Seek 方法原来只能在读 lob 模式下使用，现在该方法支持在创建的 lob 或写 lob 模式下使用。

SequoiaDB::CollectionSpace 添加接口

 * alter：对集合空间的属性进行修改
 * setAttributes：对集合空间的属性进行修改
 * setDomain：修改集合空间所属的域
 * removeDomain：移除集合空间所属的域

SequoiaDB::Domain 添加接口

 * addGroups：向域中添加数据组
 * setGroups：对域设置数据组
 * removeGroups：移除属于域的某些数据组
 * setAttributes：设置域的属性

**Version 2.9**

 * SequoiaDB::Sequoiadb 添加接口 sync，控制数据持久化

 * SequoiaDB::ReplicaGroup 废弃 GetNodeNum 接口，该接口描述的节点状态信息不准确

 * SequoiaDB::Node 废弃 GetStatus 接口，该接口描述的节点状态信息不准确。：

**Version 1.12**

* 添加使用 SSL 连接数据库的接口