此部分是 [C API](api/c/html/index.html) 相关文档。

> **Note:** 
>
> * 删除接口 - 不再兼容 
> * 废弃接口 - 保持兼容性

##历史更新情况##

**Version 2.10**

添加收集数据统计信息接口：

 * sdbAnalyze：收集指定对象的统计信息

添加修改接口

 * sdbEnableSharding：对集合启用分区功能
 * sdbDisableSharding：对集合关闭分区功能
 * sdbEnableCompression：对集合启用压缩功能
 * sdbDisableCompression：对集合关闭压缩功能
 * sdbCLSetAttributes：对集合的属性进行修改
 * sdbAlterCollectionSpace：对集合空间的属性进行修改
 * sdbCSSetAttributes：对集合空间的属性进行修改
 * sdbCSSetDomain：修改集合空间所属的域
 * sdbCSRemoveDomain：移除集合空间所属的域
 * sdbDomainAddGroups：向域中添加数据组
 * sdbDomainSetGroups：对域设置数据组
 * sdbDomainRemoveGroups：移除属于域的某些数据组
 * sdbDomainSetAttributes：设置域的属性

**Version 1.10**

添加获取查询访问计划的接口：

 * sdbExplain：获取查询的访问计划

添加用于大对象（lob）操作的接口

 * sdbListLobs：列出集合中的所有 lob
 * sdbOpenLob：创建或打开一个 lob
 * sdbCloseLob：关闭一个 lob
 * sdbRemoveLob：删除一个 lob
 * sdbSeekLob：设置读起始位置，该版本中，seek 只用于读操作
 * sdbReadLob：从 lob 中读取数据
 * sdbWriteLob：把数据写入 lob
 * sdbGetLobSize：获取 lob 的大小
 * sdbGetLobCreateTime：获取 lob 的创建时间

**Version 1.8**

新添加接口：

 * sdbConnect1：可提供多个地址，接口随机选择一个有效的地址连接
 * sdbCreateCollectionSpaceV2：提供一个 bson 的选项，使创建集合空间更加灵活
 * sdbAlterCollection：修改集合（表）属性
 * sdbCreateDomain：创建域 
 * sdbDropDomain：删除域 
 * sdbGetDomain：获取域句柄 
 * sdbListDomains：列出所有域 
 * sdbReleaseDomain：删除域句柄 
 * sdbAlterDomain：更改域属性 

**Version 1.6**

* 使用 sdbNodeHandle 来取代原来的 sdbReplicaNodeHandle。sdbReplicaNodeHandle 将在 version 2.x 中被弃用。

* 使用概念“node”取代原来的“replica node”，和“replica node”相关的 API 接口将保留，直到 version 2.x 会被弃用。

更多详情可查看辅助 API [BASE64C API](api/bson/html/base64c_8h.html) 和 [JSTOBSON API](api/bson/html/jstobs_8h.html)。
