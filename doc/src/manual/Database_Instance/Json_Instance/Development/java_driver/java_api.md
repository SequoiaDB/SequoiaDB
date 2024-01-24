此部分是相关 [Java API](api/java/html/index.html) 文档。

[^_^]:
 > **Note:** 
 >
 > * 删除接口 - 不再兼容 
 > * 废弃接口 - 保持兼容性

[^_^]:
 ##历史更新情况##

 **Version 2.10**
 
 com.sequoiadb.base.Sequoiadb 内容变更： 
 
  * 删除 getConnection 方法，IConnection 是内部网络通信接口，不再对外开放
  * 删除 getDataCenter 方法
  * 删除 setServerAddress 方法，该方法无意义
  * 废弃 getServerAddress 方法，增加 getHost、getPort 方法
  * 废弃 changeConnectionOptions 方法
  * 废弃 disconnect 方法
  * 废弃 com.sequoiadb.net.ConfigOptions 相关的构造方法，增加 com.sequoiadb.base.ConfigOptions 相关的构造方法
  * 废弃 isEndianConvert 方法，增加 getByteOrder 方法
  * closeAllCursors 方法在连接已关闭时不再报错
  * 实现 java.io.Closeable 接口，增加 close 方法，在 JDK1.7 上支持资源自动释放。新增的 close 方法取代 disconnect 接口
  * 增加 getLastUseTime 方法，该接口主要被数据源使用
  * 增加 close 方法取代原来 disconnect 的功能
  * 增加 sync 方法控制数据持久化
  * setSessionAttr 接口增加可以从多个 Instance 中选出目标 Instance 的功能
 
 com.sequoiadb.base.DBCollection 内容变更：
 
  * getCollection 方法要获取的 collection 不存在时，不再返回 null 而是抛出异常
  * 增加 openLob(ObjectId id, int mode) 方法，其中 mode 取值为 DBLob.SDB_LOB_READ 或 DBLob.SDB_LOB_WRITE
  * 增加 truncateLob 方法
  * 增加 enableSharding 方法，对集合启用分区功能
  * 增加 disableSharding 方法，对集合关闭分区功能
  * 增加 enableCompression 方法，对集合启用压缩功能
  * 增加 disableCompression 方法，对集合关闭压缩功能
  * 增加 setAttributes 方法，对集合的属性进行修改
 
 com.sequoiadb.base.DBCursor 内容变更：
 
  * 实现 java.io.Closeable 接口，在 JDK1.7 上支持资源自动释放
  * getNext 和 getNextRaw 方法可以混合交替使用 
  * 废弃 hasNextRaw 接口，可使用 hasNext 取代该接口 
 
 com.sequoiadb.base.DBLob 内容变更：
 
  * 增加 lock 方法
  * 增加 lockAndSeek 方法
  * 增加 getModificationTime 方法
  * seek 方法原来只能在读 lob 模式下使用，现在该方法支持在创建的 lob 或写 lob 模式下使用
 
 com.sequoiadb.base.Domain 内容变更
 
  * 修复 isDomainExist 接口可能存在游标泄露的情况
  * 增加 addGroups 方法，向域中添加数据组
  * 增加 setGroups 方法，对域设置数据组
  * 增加 removeGroups 方法，移除属于域的某些数据组
  * 增加 setAttributes 方法，设置域的属性
 
 com.sequoiadb.base.ReplicaGroup 内容变更：
 
  * 废弃 getNodeNum 接口，该接口描述的节点状态信息不准确
  * getSlave 方法增加可指定节点位置的参数
 
 com.sequoiadb.datasource.DatasourceOptions 内容变更
 
  * setSyncCoordInterval(int syncCoordInterval) 接口正常的输入参数 syncCoordInterval 的值若小于60,000，该接口自动将输入值改为60,000
  * 增加 getPreferedInstance/setPreferedInstance 接口，使连接池支持设置会话属性
 
 com.sequoiadb.base.CollectionSpace 添加接口
 
  * alter：对集合空间的属性进行修改
  * setAttributes：对集合空间的属性进行修改
  * setDomain：修改集合空间所属的域
  * removeDomain：移除集合空间所属的域
 
 com.sequoiadb.base.Node 内容变更
 
  * 废弃 getStatus 接口，该接口描述的节点状态信息不准确
 
 其余内容变更
 
  * 删除 com.sequoiadb.base.DataCenter 接口，待相关功能发布之后再提供接口
  * org.bson.BSONObject 实现 java.io.Serializable 接口
  * org.bson.types.BSONTimestamp 支持从 java.util.Date 和 java.sql.Timestamp 构造，并增加转换为 java.util.Date 和 java.sql.Timestamp 的方法
  * 废弃 com.sequoiadb.base.SequoiadbDatasource 类，增加com.sequoiadb.datasource.SequoiadbDatasource 类
  * BSONObject 支持将 java.sql.Timestamp 编码为 timestamp 类型
 
 **Version 1.10**
 
 DBCollection 类添加接口 
 
  * createLob：创建一个大对象 
  * openLob：打开一个已存在的大对象 
  * removeLob：删除一个大对象 
  * listLobs：列出所有大对象 
  * explain：获取执行访问计划 
 
 增加大对象类 DBLob 及相关接口，用于操作大对象 
 
  * write：向一个大对象写入数据 
  * read：从大对象中读取数据 
  * seek：指定读取数据的偏移 
  * close：关闭一个大对象 
  * getID：获取大对象的标识 ID 
  * getSize：获取大对象的大小 
  * getCreateTime：获取大对象的创建时间 
 
 **Version 1.8**
 
 Sequoiadb 类添加接口
 
  * isValid：判断当前连接是否有效 
  * createCollectionSpace：提供一个 BSONObject 的选项，使创建集合空间更加灵活 
  * backup：备份支持更多的选项 
  * evalJS：执行javascript代码 
  * createDomain：创建域 
  * getDomain：获取域
  * dropDomain：删除域
  * isDomainExist：域是否存在
  * listDomain：列出所有域
 
 DBCollection 类添加接口
 
  * alterCollection：修改集合（表）属性
  * save：可使用默认的主键 _id 或者指定其他主键，同时插入或更新多条记录
  * setMainKeys：设置主键，此接口只与 save 接口配合使用，通过此接口设置的主键并不对其他接口有效
 
 SequoiadbDatasource 类添加接口
 
  * SequoiadbDatasource：可提供多个地址的构造器，便于机器负载均衡
  * getIdleConnNum：获取当前可用的连接数量。
  * getUsedConnNum：获取当前已使用的连接数量。
  * getNormalAddrNum：获取当前正常的地址数量。
  * getAbnormalAddrNum：获取当前异常的地址数量。
 
 SequoiadbOption 类添加接口
 
  * setRecaptureConnPeriod：设置周期检测异常地址是否重新可用的时间
  * getRecaptureConnPeriod：获取周期检测异常地址是否重新可用的时间
 
 其他内容变更
 
  * 添加 Domain 类用于与域相关的操作
 
 **Version 1.6**
 
  * 添加类 Node 来取代原来的类 ReplicaGroup
  * 类 ReplicaNode 以及相关的方法将在 version 2.x 中被弃用

