Sdb 类主要用于连接和操作 SequoiaDB 巨杉数据库，包含的函数函数如下：

| 名称 | 描述 |
|------|------|
| [Sdb()][Sdb] | SequoiaDB 连接对象 |
| [analyze()][analyze] | 收集统计信息 |
| [backup()][backup] | 备份数据库 |
| [cancelTask()][cancelTask] | 取消任务 |
| [close()][close] | 关闭数据库连接 |
| [createCataRG()][createCataRG] | 新建编目复制组 |
| [createCoordRG()][createCoordRG] | 创建协调复制组 |
| [createCS()][createCS] | 创建集合空间 |
| [createDataSource()][createDataSource] | 创建数据源 |
| [createDomain()][createDomain] | 创建域 |
| [createProcedure()][createProcedure] | 创建存储过程 |
| [createRG()][createRG] | 新建复制组 |
| [createSequence()][createSequence] | 创建序列对象 |
| [createSpareRG()][createSpareRG]| 创建热备组 |
| [createUsr()][createUsr] | 创建数据库用户 |
| [dropCS()][dropCS] | 删除一个已存在的集合空间 |
| [dropDataSource()][dropDataSource] | 删除数据源 |
| [dropDomain()][dropDomain] | 删除域 |
| [dropSequence()][dropSequence] | 删除指定的序列 |
| [dropUsr()][dropUsr] | 删除数据库用户 |
| [eval()][eval] | 调用存储过程 |
| [exec()][exec] | 执行 SQL 的 select 语句 |
| [execUpdate()][execUpdate] | 执行除 select 以外的 SQL 语句 |
| [flushConfigure()][flushConfigure] | 将节点内存中的配置刷盘至配置文件 |
| [forceSession()][forceSession] | 终止指定会话的当前操作 |
| [forceStepUp()][forceStepUp] | 强制将备节点升级为主节点 |
| [getCataRG()][getCataRG] | 获取编目复制组的引用 |
| [getCoordRG()][getCoordRG] | 获取协调复制组的引用 |
| [getCS()][getCS] | 获取指定集合空间 |
| [getDataSource()][getDataSource] | 获取数据源的引用 |
| [getDomain()][getDomain] | 获取指定域 |
| [getRecycleBin()][getRecycleBin] | 获取回收站的引用 |
| [getRG()][getRG] | 获取指定复制组 |
| [getSequence()][getSequence] | 获取指定的序列对象 |
| [getSessionAttr()][getSessionAttr] | 获取会话属性 |
| [getSpareRG()][getSpareRG] | 获取备份组的引用 |
| [invalidateCache()][invalidateCache] | 清除节点的缓存信息 |
| [list()][list] | 获取列表 |
| [listBackup()][listBackup] | 枚举数据库备份 |
| [listCollections()][listCollections] | 枚举集合信息 |
| [listCollectionSpaces()][listCollectionSpaces] | 枚举集合空间信息 |
| [listDataSources()][listDataSources] | 查看数据源的元数据信息 |
| [listDomains()][listDomains] | 枚举用户创建的域 |
| [listProcedures()][listProcedures] | 枚举存储过程 |
| [listReplicaGroups()][listReplicaGroups] | 枚举复制组信息 |
| [listSequences()][listSequences] | 枚举序列信息 |
| [listTasks()][listTasks] | 枚举后台任务 |
| [removeBackup()][removeBackup] | 删除数据库备份 |
| [removeCataRG()][removeCataRG] | 删除编目复制组 |
| [removeCoordRG()][removeCoordRG] | 删除数据库中的协调复制组 |
| [removeSpareRG()][removeSpareRG] | 删除数据库中的 SYSSpare 组 |
| [removeProcedure()][removeProcedure] | 删除指定的函数名 |
| [removeRG()][removeRG] | 删除复制组 |
| [renameCS()][renameCS] | 集合空间改名 |
| [renameSequence()][renameSequence] | 修改序列名 |
| [resetSnapshot()][resetSnapshot] | 重置快照 |
| [reloadConf()][reloadConf] | 重新加载配置文件 |
| [deleteConf()][deleteConf] | 删除配置 |
| [setSessionAttr()][setSessionAttr] | 设置会话属性 |
| [snapshot()][snapshot] | 获取快照 |
| [startRG()][startRG] | 启动复制组 |
| [stopRG()][stopRG] | 停止复制组 |
| [sync()][sync] | 持久化数据和日志到磁盘 |
| [setPDLevel()][setPDLevel] | 动态设置节点的诊断日志级别 |
| [traceOff()][traceOff] | 关闭数据库引擎跟踪功能 |
| [traceOn()][traceOn] | 开启数据库引擎跟踪功能 |
| [traceResume()][traceResume] | 重新开启断点跟踪程序 |
| [traceStatus()][traceStatus] | 查看当前程序跟踪的状态 |
| [transBegin()][transBegin] | 开启事务 |
| [transCommit()][transCommit] | 事务提交 |
| [transRollback()][transRollback] | 事务回滚 |
| [updateConf()][updateConf] | 更新节点配置 |
| [waitTasks()][waitTasks] | 同步等待指定任务结束或取消 |
| [memTrim()][memTrim] | 空闲内存回收 |

[^_^]:
     本文使用的所有引用及链接
[Sdb]:manual/Manual/Sequoiadb_Command/Sdb/Sdb.md
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[backup]:manual/Manual/Sequoiadb_Command/Sdb/backup.md
[cancelTask]:manual/Manual/Sequoiadb_Command/Sdb/cancelTask.md
[close]:manual/Manual/Sequoiadb_Command/Sdb/close.md
[createCataRG]:manual/Manual/Sequoiadb_Command/Sdb/createCataRG.md
[createCoordRG]:manual/Manual/Sequoiadb_Command/Sdb/createCoordRG.md
[createCS]:manual/Manual/Sequoiadb_Command/Sdb/createCS.md
[createDataSource]:manual/Manual/Sequoiadb_Command/Sdb/createDataSource.md
[createDomain]:manual/Manual/Sequoiadb_Command/Sdb/createDomain.md
[createProcedure]:manual/Manual/Sequoiadb_Command/Sdb/createProcedure.md
[createRG]:manual/Manual/Sequoiadb_Command/Sdb/createRG.md
[createSequence]:manual/Manual/Sequoiadb_Command/Sdb/createSequence.md
[createSpareRG]:manual/Manual/Sequoiadb_Command/Sdb/createSpareRG.md
[createUsr]:manual/Manual/Sequoiadb_Command/Sdb/createUsr.md
[dropCS]:manual/Manual/Sequoiadb_Command/Sdb/dropCS.md
[dropDataSource]:manual/Manual/Sequoiadb_Command/Sdb/dropDataSource.md
[dropDomain]:manual/Manual/Sequoiadb_Command/Sdb/dropDomain.md
[dropSequence]:manual/Manual/Sequoiadb_Command/Sdb/dropSequence.md
[dropUsr]:manual/Manual/Sequoiadb_Command/Sdb/dropUsr.md
[eval]:manual/Manual/Sequoiadb_Command/Sdb/eval.md
[exec]:manual/Manual/Sequoiadb_Command/Sdb/exec.md
[execUpdate]:manual/Manual/Sequoiadb_Command/Sdb/execUpdate.md
[flushConfigure]:manual/Manual/Sequoiadb_Command/Sdb/flushConfigure.md
[forceSession]:manual/Manual/Sequoiadb_Command/Sdb/forceSession.md
[forceStepUp]:manual/Manual/Sequoiadb_Command/Sdb/forceStepUp.md
[getCataRG]:manual/Manual/Sequoiadb_Command/Sdb/getCataRG.md
[getCoordRG]:manual/Manual/Sequoiadb_Command/Sdb/getCoordRG.md
[getCS]:manual/Manual/Sequoiadb_Command/Sdb/getCS.md
[getDataSource]:manual/Manual/Sequoiadb_Command/Sdb/getDataSource.md
[getDomain]:manual/Manual/Sequoiadb_Command/Sdb/getDomain.md
[getRecycleBin]:manual/Manual/Sequoiadb_Command/Sdb/getRecycleBin.md
[getRG]:manual/Manual/Sequoiadb_Command/Sdb/getRG.md
[getSequence]:manual/Manual/Sequoiadb_Command/Sdb/getSequence.md
[getSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/getSessionAttr.md
[getSpareRG]:manual/Manual/Sequoiadb_Command/Sdb/getSpareRG.md
[invalidateCache]:manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md
[list]:manual/Manual/Sequoiadb_Command/Sdb/list.md
[listBackup]:manual/Manual/Sequoiadb_Command/Sdb/listBackup.md
[listCollections]:manual/Manual/Sequoiadb_Command/Sdb/listCollections.md
[listCollectionSpaces]:manual/Manual/Sequoiadb_Command/Sdb/listCollectionSpaces.md
[listDataSources]:manual/Manual/Sequoiadb_Command/Sdb/listDataSources.md
[listDomains]:manual/Manual/Sequoiadb_Command/Sdb/listDomains.md
[listProcedures]:manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md
[listReplicaGroups]:manual/Manual/Sequoiadb_Command/Sdb/listReplicaGroups.md
[listSequences]:manual/Manual/Sequoiadb_Command/Sdb/listSequences.md
[listTasks]:manual/Manual/Sequoiadb_Command/Sdb/listTasks.md
[removeBackup]:manual/Manual/Sequoiadb_Command/Sdb/removeBackup.md
[removeCataRG]:manual/Manual/Sequoiadb_Command/Sdb/removeCataRG.md
[removeCoordRG]:manual/Manual/Sequoiadb_Command/Sdb/removeCoordRG.md
[removeSpareRG]:manual/Manual/Sequoiadb_Command/Sdb/removeSpareRG.md
[removeProcedure]:manual/Manual/Sequoiadb_Command/Sdb/removeProcedure.md
[removeRG]:manual/Manual/Sequoiadb_Command/Sdb/removeRG.md
[renameCS]:manual/Manual/Sequoiadb_Command/Sdb/renameCS.md
[renameSequence]:manual/Manual/Sequoiadb_Command/Sdb/renameSequence.md
[resetSnapshot]:manual/Manual/Sequoiadb_Command/Sdb/resetSnapshot.md
[reloadConf]:manual/Manual/Sequoiadb_Command/Sdb/reloadConf.md
[deleteConf]:manual/Manual/Sequoiadb_Command/Sdb/deleteConf.md
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
[startRG]:manual/Manual/Sequoiadb_Command/Sdb/startRG.md
[stopRG]:manual/Manual/Sequoiadb_Command/Sdb/stopRG.md
[sync]:manual/Manual/Sequoiadb_Command/Sdb/sync.md
[setPDLevel]:manual/Manual/Sequoiadb_Command/Sdb/setPDLevel.md
[traceOff]:manual/Manual/Sequoiadb_Command/Sdb/traceOff.md
[traceOn]:manual/Manual/Sequoiadb_Command/Sdb/traceOn.md
[traceResume]:manual/Manual/Sequoiadb_Command/Sdb/traceResume.md
[traceStatus]:manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md
[transBegin]:manual/Manual/Sequoiadb_Command/Sdb/transBegin.md
[transCommit]:manual/Manual/Sequoiadb_Command/Sdb/transCommit.md
[transRollback]:manual/Manual/Sequoiadb_Command/Sdb/transRollback.md
[updateConf]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
[waitTasks]:manual/Manual/Sequoiadb_Command/Sdb/waitTasks.md
[memTrim]:manual/Manual/Sequoiadb_Command/Sdb/memTrim.md
