SdbCollection 类主要用于操作集合，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [aggregate()][aggregate] | 计算集合中数据的聚合值 |
| [alter()][alter] | 修改集合的属性 |
| [attachCL()][attachCL] | 挂载子分区集合 |
| [count()][count] | 统计当前集合符合条件的记录总数 |
| [createAutoIncrement()][createAutoIncrement] | 创建自增字段 |
| [createIdIndex()][createIdIndex] | 创建 $id 索引 |
| [createIndex()][createIndex] | 创建索引 |
| [createLobID()][createLobID] | 创建大对象 ID |
| [deleteLob()][deleteLob] | 删除集合中的大对象 |
| [detachCL()][detachCL] | 从主分区集合中分离出子分区集合 |
| [disableCompression()][disableCompression] | 修改集合的属性关闭压缩功能 |
| [disableSharding()][disableSharding] | 修改集合的属性关闭分区功能 |
| [dropAutoIncrement()][dropAutoIncrement] | 删除自增字段 |
| [dropIdIndex()][dropIdIndex] | 删除集合中的 $id 索引 |
| [dropIndex()][dropIndex] | 删除集合中指定的索引 |
| [enableCompression()][enableCompression] | 开启集合的压缩功能或者修改集合的压缩算法 |
| [enableSharding()][enableSharding] | 修改集合的属性开启分区属性 | 
| [find()][find] | 查询记录 |
| [findOne()][findOne] | 查询符合条件的一条记录 |
| [getDetail()][getDetail] | 获取集合具体信息 |
| [getIndex()][getIndex] | 获取指定索引 |
| [getIndexStat()][getIndexStat] | 获取指定索引的统计信息 |
| [getLob()][getLob] | 读取大对象 |
| [getLobDetail()][getLobDetail] | 获取大对象被读写访问的详细信息 |
| [insert()][insert] | 将单条或者批量记录插入当前集合 |
| [listIndexes()][listIndexes] | 枚举集合下的索引信息 |
| [listLobs()][listLobs] | 列举集合中的大对象 |
| [putLob()][putLob] | 在集合中插入大对象 |
| [remove()][remove] | 删除集合中的记录 |
| [setAttributes()][setAttributes] | 修改集合的属性 |
| [split()][split] | 切分数据记录 |
| [splitAsync()][splitAsync] | 异步切分数据记录 |
| [truncate()][truncate] | 删除集合内所有数据 |
| [truncateLob()][truncateLob] | 截短集合中的大对象 |
| [update()][update] | 更新集合记录 |
| [upsert()][upsert] | 更新集合记录 |

[^_^]:
     本文使用的所有引用及链接
[aggregate]:manual/Manual/Sequoiadb_Command/SdbCollection/aggregate.md
[alter]:manual/Manual/Sequoiadb_Command/SdbCollection/alter.md
[attachCL]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md
[count]:manual/Manual/Sequoiadb_Command/SdbCollection/count.md
[createAutoIncrement]:manual/Manual/Sequoiadb_Command/SdbCollection/createAutoIncrement.md
[createIdIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIdIndex.md
[createIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[createLobID]:manual/Manual/Sequoiadb_Command/SdbCollection/createLobID.md
[deleteLob]:manual/Manual/Sequoiadb_Command/SdbCollection/deleteLob.md
[detachCL]:manual/Manual/Sequoiadb_Command/SdbCollection/detachCL.md
[disableCompression]:manual/Manual/Sequoiadb_Command/SdbCollection/disableCompression.md
[disableSharding]:manual/Manual/Sequoiadb_Command/SdbCollection/disableSharding.md
[dropAutoIncrement]:manual/Manual/Sequoiadb_Command/SdbCollection/dropAutoIncrement.md
[dropIdIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIdIndex.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md
[enableCompression]:manual/Manual/Sequoiadb_Command/SdbCollection/enableCompression.md
[enableSharding]:manual/Manual/Sequoiadb_Command/SdbCollection/enableSharding.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[findOne]:manual/Manual/Sequoiadb_Command/SdbCollection/findOne.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbCollection/getDetail.md
[getIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/getIndex.md
[getIndexStat]:manual/Manual/Sequoiadb_Command/SdbCollection/getIndexStat.md
[getLob]:manual/Manual/Sequoiadb_Command/SdbCollection/getLob.md
[getLobDetail]:manual/Manual/Sequoiadb_Command/SdbCollection/getLobDetail.md
[insert]:manual/Manual/Sequoiadb_Command/SdbCollection/insert.md
[listIndexes]:manual/Manual/Sequoiadb_Command/SdbCollection/listIndexes.md
[listLobs]:manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md
[putLob]:manual/Manual/Sequoiadb_Command/SdbCollection/putLob.md
[remove]:manual/Manual/Sequoiadb_Command/SdbCollection/remove.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md
[splitAsync]:manual/Manual/Sequoiadb_Command/SdbCollection/splitAsync.md
[truncate]:manual/Manual/Sequoiadb_Command/SdbCollection/truncate.md
[truncateLob]:manual/Manual/Sequoiadb_Command/SdbCollection/truncateLob.md
[update]:manual/Manual/Sequoiadb_Command/SdbCollection/update.md
[upsert]:manual/Manual/Sequoiadb_Command/SdbCollection/upsert.md