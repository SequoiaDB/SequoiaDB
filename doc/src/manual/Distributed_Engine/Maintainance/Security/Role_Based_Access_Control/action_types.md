权限的操作定义了用户在指定资源上可以执行的操作。一个权限由一个资源和操作列表组成。该页面列出了按共同目的分组的可用操作。

# 读写操作

## find

将该操作应用在集合空间或集合资源上。

用户可执行以下命令：

- [find](manual/Manual/Sequoiadb_Command/SdbCollection/find.md)
- [findOne](manual/Manual/Sequoiadb_Command/SdbCollection/findOne.md)
- [getLob](manual/Manual/Sequoiadb_Command/SdbCollection/getLob.md)
- [count](manual/Manual/Sequoiadb_Command/SdbCollection/count.md)
- [getQueryMeta](manual/Manual/Sequoiadb_Command/SdbQuery/getQueryMeta.md)
- [getCS](manual/Manual/Sequoiadb_Command/Sdb/getCS.md)
- [getCL](manual/Manual/Sequoiadb_Command/SdbCS/getCL.md)
- [getIndex](manual/Manual/Sequoiadb_Command/SdbCollection/getIndex.md)
- [listCollections](manual/Manual/Sequoiadb_Command/SdbCS/listCollections.md)
- [listIndexes](manual/Manual/Sequoiadb_Command/SdbCollection/listIndexes.md)
- [listLobs](manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md)
- [getDetail](manual/Manual/Sequoiadb_Command/SdbCollection/getDetail.md)
- [getCollectionStat](manual/Manual/Sequoiadb_Command/SdbCollection/getCollectionStat.md)
- [getIndexStat](manual/Manual/Sequoiadb_Command/SdbCollection/getIndexStat.md)

[exec](manual/Manual/Sequoiadb_Command/Sdb/exec.md)与[aggregate](manual/Manual/Sequoiadb_Command/SdbCollection/aggregate.md)命令的查询部分需要被授予 find 操作。

 find().update() 与 find().remove() 命令的查询部分需要被授予 find 操作。

## insert

将该操作应用在集合空间或集合资源上。

用户可执行以下命令：

- [insert](manual/Manual/Sequoiadb_Command/SdbCollection/insert.md)
- [putLob](manual/Manual/Sequoiadb_Command/SdbCollection/putLob.md)

## update

将该操作应用在集合空间或集合资源上。

用户可执行以下命令：

- [update](manual/Manual/Sequoiadb_Command/SdbCollection/update.md)
- [upsert](manual/Manual/Sequoiadb_Command/SdbCollection/upsert.md) 仅发生更新
- [truncateLob](manual/Manual/Sequoiadb_Command/SdbCollection/truncateLob.md)

 如果 upsert 命令将发生插入，需要额外被授予 insert 操作。
 
 find().update() 命令需要被授予 find 和 update 操作。

## remove

将该操作应用在集合空间或集合资源上。

用户可执行以下命令：

- [remove](manual/Manual/Sequoiadb_Command/SdbCollection/remove.md)
- [deleteLob](manual/Manual/Sequoiadb_Command/SdbCollection/deleteLob.md)

 find().remove() 命令需要被授予 find 和 remove 操作。

## getDetail

将该操作应用在集合空间或集合资源上。

用户可执行以下命令：

- [count](manual/Manual/Sequoiadb_Command/SdbCollection/count.md)
- [getQueryMeta](manual/Manual/Sequoiadb_Command/SdbQuery/getQueryMeta.md)
- [getCS](manual/Manual/Sequoiadb_Command/Sdb/getCS.md)
- [getCL](manual/Manual/Sequoiadb_Command/SdbCS/getCL.md)
- [getIndex](manual/Manual/Sequoiadb_Command/SdbCollection/getIndex.md)
- [listCollections](manual/Manual/Sequoiadb_Command/SdbCS/listCollections.md)
- [listIndexes](manual/Manual/Sequoiadb_Command/SdbCollection/listIndexes.md)
- [listLobs](manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md)
- [getDetail](manual/Manual/Sequoiadb_Command/SdbCollection/getDetail.md)
- [getCollectionStat](manual/Manual/Sequoiadb_Command/SdbCollection/getCollectionStat.md)
- [getIndexStat](manual/Manual/Sequoiadb_Command/SdbCollection/getIndexStat.md)

# 集合管理操作

## alterCL

将该操作应用在集合资源上。

用户可执行以下命令：

- [alter (on collection)](manual/Manual/Sequoiadb_Command/SdbCollection/alter.md)
- [createIdIndex](manual/Manual/Sequoiadb_Command/SdbCollection/createIdIndex.md)
- [dropIdIndex](manual/Manual/Sequoiadb_Command/SdbCollection/dropIdIndex.md)
- [createAutoIncrement](manual/Manual/Sequoiadb_Command/SdbCollection/createAutoIncrement.md)
- [dropAutoIncrement](manual/Manual/Sequoiadb_Command/SdbCollection/dropAutoIncrement.md)
- [enableSharding](manual/Manual/Sequoiadb_Command/SdbCollection/enableSharding.md)
- [disableSharding](manual/Manual/Sequoiadb_Command/SdbCollection/disableSharding.md)
- [enableCompression](manual/Manual/Sequoiadb_Command/SdbCollection/enableCompression.md)
- [disableCompression](manual/Manual/Sequoiadb_Command/SdbCollection/disableCompression.md)
- [setAttributes (on collection)](manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md)

## createIndex

将该操作应用在集合资源上。

用户可执行[createIndex](manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md)和[createIndexAsync](manual/Manual/Sequoiadb_Command/SdbCollection/createIndexAsync.md)命令。

## dropIndex

将该操作应用在集合资源上。

用户可执行[dropIndex](manual/Manual/Sequoiadb_Command/SdbCollection/dropIndex.md)和[dropIndexAsync](manual/Manual/Sequoiadb_Command/SdbCollection/dropIndexAsync.md)命令。

## truncate

将该操作应用在集合资源上。

用户可执行[truncate](manual/Manual/Sequoiadb_Command/SdbCollection/truncate.md)命令。

## testCL

将该操作应用在集合资源上。

用户可执行[getCL](manual/Manual/Sequoiadb_Command/SdbCS/getCL.md)命令。

# 集合空间管理操作

## alterCS

将该操作应用在集合空间资源上。

用户可执行以下命令：

- [alter (on collection space)](manual/Manual/Sequoiadb_Command/SdbCS/alter.md)
- [setDomain](manual/Manual/Sequoiadb_Command/SdbCS/setDomain.md)
- [removeDomain](manual/Manual/Sequoiadb_Command/SdbCS/removeDomain.md)
- [setAttributes (on collection space)](manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md)

## createCL

将该操作应用在集合空间资源上。

用户可执行[createCL](manual/Manual/Sequoiadb_Command/SdbCS/createCL.md)命令。

## dropCL

将该操作应用在集合空间资源上。

用户可执行[dropCL](manual/Manual/Sequoiadb_Command/SdbCS/dropCL.md)命令。

## renameCL

将该操作应用在集合空间资源上。

用户可执行[renameCL](manual/Manual/Sequoiadb_Command/SdbCS/renameCL.md)命令。

## testCS

将该操作应用在集合空间资源上。

用户可执行[getCS](manual/Manual/Sequoiadb_Command/Sdb/getCS.md)命令。

# 回收站操作

## alterBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行以下命令：

- [alter](manual/Manual/Sequoiadb_Command/SdbRecycleBin/alter.md)
- [setAttributes](manual/Manual/Sequoiadb_Command/SdbRecycleBin/setAttributes.md)
- [disable](manual/Manual/Sequoiadb_Command/SdbRecycleBin/disable.md)
- [enable](manual/Manual/Sequoiadb_Command/SdbRecycleBin/enable.md)

## countBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行[count](manual/Manual/Sequoiadb_Command/SdbRecycleBin/count.md)命令。

## dropAllBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行[dropAll](manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropAll.md)命令。

## dropItemBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行[dropItem](manual/Manual/Sequoiadb_Command/SdbRecycleBin/dropItem.md)命令。

## getDetailBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行[getDetail](manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md)命令。

## listBin

将该操作应用在集群资源上。

用户可执行 list(SDB_LIST_RECYCLEBIN) 命令。

用户可在 SdbRecycleBin 对象上执行[list](manual/Manual/Sequoiadb_Command/SdbRecycleBin/list.md)命令。

## returnItemBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行[returnItem](manual/Manual/Sequoiadb_Command/SdbRecycleBin/returnItem.md)和[returnItemToName](manual/Manual/Sequoiadb_Command/SdbRecycleBin/returnItemToName.md)命令。

## snapshotBin

将该操作应用在集群资源上。

用户可在 SdbRecycleBin 对象上执行[snapshot](manual/Manual/Sequoiadb_Command/SdbRecycleBin/snapshot.md)命令。

# 部署管理操作

## createRG

将该操作应用在集群资源上。

用户可执行[createRG](manual/Manual/Sequoiadb_Command/Sdb/createRG.md)命令。

## forceStepUp

将该操作应用在集群资源上。

用户可执行[forceStepUp](manual/Manual/Sequoiadb_Command/Sdb/forceStepUp.md)命令。

## getRG

将该操作应用在集群资源上。

用户可执行以下命令：

- [getRG](manual/Manual/Sequoiadb_Command/Sdb/getRG.md)
- [listGroups](manual/Manual/Sequoiadb_Command/SdbDomain/listGroups.md)

用户可在 SdbReplicaGroup 对象上执行以下命令：

- [getNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getNode.md)
- [getDetailObj](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getDetailObj.md)
- [getMaster](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getMaster.md)
- [getSlave](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getSlave.md)

## removeRG

将该操作应用在集群资源上。

用户可执行[removeRG](manual/Manual/Sequoiadb_Command/Sdb/removeRG.md)命令。

## reloadConf

将该操作应用在集群资源上。

用户可执行[reloadConf](manual/Manual/Sequoiadb_Command/Sdb/reloadConf.md)命令。

## deleteConf

将该操作应用在集群资源上。

用户可执行[deleteConf](manual/Manual/Sequoiadb_Command/Sdb/deleteConf.md)命令。

## updateConf

将该操作应用在集群资源上。

用户可执行[updateConf](manual/Manual/Sequoiadb_Command/Sdb/updateConf.md)命令。

## createNode

将该操作应用在集群资源上。

用户可在 SdbReplicaGroup 对象上执行[createNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md)和[attachNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/attachNode.md)命令。

## reelect

将该操作应用在集群资源上。

用户可在 SdbReplicaGroup 对象上执行[reelect](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md)命令。

## removeNode

将该操作应用在集群资源上。

用户可在 SdbReplicaGroup 对象上执行[removeNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/removeNode.md)和[detachNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/detachNode.md)命令。

## getNode

将该操作应用在集群资源上。

用户可执行以下命令：

- [getRG](manual/Manual/Sequoiadb_Command/Sdb/getRG.md)
- [listGroups](manual/Manual/Sequoiadb_Command/SdbDomain/listGroups.md)

用户可在 SdbReplicaGroup 对象上执行以下命令：

- [getNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getNode.md)
- [getDetailObj](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getDetailObj.md)
- [getMaster](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getMaster.md)
- [getSlave](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getSlave.md)

## startRG

将该操作应用在集群资源上。

用户可在 SdbReplicaGroup 对象上执行[start](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md)命令。

## stopRG

将该操作应用在集群资源上。

用户可在 SdbReplicaGroup 对象上执行[stop](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stop.md)命令。

## alterRG

将该操作应用在集群资源上。

用户可在`SdbReplicaGroup`对象上执行以下命令：

- [setActiveLocation](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/setActiveLocation.md)
- [setAttributes](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/setAttributes.md)
- [startCriticalMode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startCriticalMode.md)
- [stopCriticalMode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stopCriticalMode.md)
- [startMaintenanceMode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startMaintenanceMode.md)
- [stopMaintenanceMode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stopMaintenanceMode.md)

## startNode

将该操作应用在集群资源上。

用户可在 SdbNode 对象上执行[start](manual/Manual/Sequoiadb_Command/SdbNode/start.md)命令。

## stopNode

将该操作应用在集群资源上。

用户可在 SdbNode 对象上执行[stop](manual/Manual/Sequoiadb_Command/SdbNode/stop.md)命令。

## alterNode

将该操作应用在集群资源上。

用户可在`SdbNode`对象上执行以下命令：

- [setAttributes](manual/Manual/Sequoiadb_Command/SdbNode/setAttributes.md)
- [setLocation](manual/Manual/Sequoiadb_Command/SdbNode/setLocation.md)

# 分片操作

## attachCL

将该操作应用在集合资源上。

用户执行[attachCL](manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md)命令时需要被授予 attachCL 操作。另外在主表资源上需要被授予 find 操作，在子表资源上需要被授予 find ， insert ， update 和 remove 操作。

## copyIndex

将该操作应用在集合资源上。

用户可执行[copyIndex](manual/Manual/Sequoiadb_Command/SdbCollection/copyIndex.md)和[copyIndexAsync](manual/Manual/Sequoiadb_Command/SdbCollection/copyIndexAsync.md)命令。

## detachCL

将该操作应用在集合资源上。

用户可执行[detachCL](manual/Manual/Sequoiadb_Command/SdbCollection/detachCL.md)命令。

## split

将该操作应用在集合资源上。

用户可执行[split](manual/Manual/Sequoiadb_Command/SdbCollection/split.md)命令。

# 服务器管理操作

## backup

将该操作应用在集群资源上。

用户可执行[backup](manual/Manual/Sequoiadb_Command/Sdb/backup.md)命令。

## createCS

将该操作应用在集群资源上。

用户可执行[createCS](manual/Manual/Sequoiadb_Command/Sdb/createCS.md)命令。

## dropCS

将该操作应用在集群资源上。

用户可执行[dropCS](manual/Manual/Sequoiadb_Command/Sdb/dropCS.md)命令。

## cancelTask

将该操作应用在集群资源上。

用户可执行[cancelTask](manual/Manual/Sequoiadb_Command/Sdb/cancelTask.md)命令。

## createRole

将该操作应用在集群资源上。

用户可执行[createRole](manual/Manual/Sequoiadb_Command/Sdb/createRole.md)命令。

## dropRole

将该操作应用在集群资源上。

用户可执行[dropRole](manual/Manual/Sequoiadb_Command/Sdb/dropRole.md)命令。

## listRoles

将该操作应用在集群资源上。

用户可执行[listRoles](manual/Manual/Sequoiadb_Command/Sdb/listRoles.md)命令。

## updateRole

将该操作应用在集群资源上。

用户可执行[updateRole](manual/Manual/Sequoiadb_Command/Sdb/updateRole.md)命令。

## grantPrivilegesToRole

将该操作应用在集群资源上。

用户可执行[grantPrivilegesToRole](manual/Manual/Sequoiadb_Command/Sdb/grantPrivilegesToRole.md)命令。

## revokePrivilegesFromRole

将该操作应用在集群资源上。

用户可执行[revokePrivilegesFromRole](manual/Manual/Sequoiadb_Command/Sdb/revokePrivilegesFromRole.md)命令。

## grantRolesToRole

将该操作应用在集群资源上。

用户可执行[grantRolesToRole](manual/Manual/Sequoiadb_Command/Sdb/grantRolesToRole.md)命令。

## revokeRolesFromRole

将该操作应用在集群资源上。

用户可执行[revokeRolesFromRole](manual/Manual/Sequoiadb_Command/Sdb/revokeRolesFromRole.md)命令。

## createUsr

将该操作应用在集群资源上。

用户可执行[createUsr](manual/Manual/Sequoiadb_Command/Sdb/createUsr.md)命令。

## dropUsr

将该操作应用在集群资源上。

用户可执行[dropUsr](manual/Manual/Sequoiadb_Command/Sdb/dropUsr.md)命令。

## grantRolesToUser

将该操作应用在集群资源上。

用户可执行[grantRolesToUser](manual/Manual/Sequoiadb_Command/Sdb/grantRolesToUser.md)命令。

## revokeRolesFromUser

将该操作应用在集群资源上。

用户可执行[revokeRolesFromUser](manual/Manual/Sequoiadb_Command/Sdb/revokeRolesFromUser.md)命令。

## createDataSource

将该操作应用在集群资源上。

用户可执行[createDataSource](manual/Manual/Sequoiadb_Command/Sdb/createDataSource.md)命令。

## createDomain

将该操作应用在集群资源上。

用户可执行[createDomain](manual/Manual/Sequoiadb_Command/Sdb/createDomain.md)命令。

## createProcedure

将该操作应用在集群资源上。

用户可执行[createProcedure](manual/Manual/Sequoiadb_Command/Sdb/createProcedure.md)命令。

## createSequence

将该操作应用在集群资源上。

用户可执行[createSequence](manual/Manual/Sequoiadb_Command/Sdb/createSequence.md)命令。

## dropDataSource

将该操作应用在集群资源上。

用户可执行[dropDataSource](manual/Manual/Sequoiadb_Command/Sdb/dropDataSource.md)命令。

## dropDomain

将该操作应用在集群资源上。

用户可执行[dropDomain](manual/Manual/Sequoiadb_Command/Sdb/dropDomain.md)命令。

## dropSequence

将该操作应用在集群资源上。

用户可执行[dropSequence](manual/Manual/Sequoiadb_Command/Sdb/dropSequence.md)命令。

## eval

将该操作应用在集群资源上。

用户可执行[eval](manual/Manual/Sequoiadb_Command/Sdb/eval.md)命令。

## flushConfigure

将该操作应用在集群资源上。

用户可执行[flushConfigure](manual/Manual/Sequoiadb_Command/Sdb/flushConfigure.md)命令。

## invalidateCache

将该操作应用在集群资源上。

用户可执行[invalidateCache](manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md)命令。

## invalidateUserCache

将该操作应用在集群资源上。

用户可执行[invalidateUserCache](manual/Manual/Sequoiadb_Command/Sdb/invalidateUserCache.md)命令。

## removeBackup

将该操作应用在集群资源上。

用户可执行[removeBackup](manual/Manual/Sequoiadb_Command/Sdb/removeBackup.md)命令。

## removeProcedure

将该操作应用在集群资源上。

用户可执行[removeProcedure](manual/Manual/Sequoiadb_Command/Sdb/removeProcedure.md)命令。

## renameCS

将该操作应用在集群资源上。

用户可执行[renameCS](manual/Manual/Sequoiadb_Command/Sdb/renameCS.md)命令。

## resetSnapshot

将该操作应用在集群资源上。

用户可执行[resetSnapshot](manual/Manual/Sequoiadb_Command/Sdb/resetSnapshot.md)命令。

## sync

将该操作应用在集群资源上。

用户可执行[sync](manual/Manual/Sequoiadb_Command/Sdb/sync.md)命令。

## alterDataSource

将该操作应用在集群资源上。

用户可执行 SdbDataSource 对象上的[alter](manual/Manual/Sequoiadb_Command/SdbDataSource/alter.md)命令。

## fetchSequence

将该操作应用在集群资源上。

用户可在 SdbSequence 对象上执行[fetch](manual/Manual/Sequoiadb_Command/SdbSequence/fetch.md)和[getNextValue](manual/Manual/Sequoiadb_Command/SdbSequence/getNextValue.md)命令。

## getSequenceCurrentValue

将该操作应用在集群资源上。

用户可在 SdbSequence 对象上执行[getCurrentValue](manual/Manual/Sequoiadb_Command/SdbSequence/getCurrentValue.md)命令。

## alterSequence

将该操作应用在集群资源上。

用户可在 SdbSequence 对象上执行以下命令：

- [restart](manual/Manual/Sequoiadb_Command/SdbSequence/restart.md)
- [setAttributes](manual/Manual/Sequoiadb_Command/SdbSequence/setAttributes.md)
- [renameSequence](manual/Manual/Sequoiadb_Command/Sdb/renameSequence.md)
- [setCurrentValue](manual/Manual/Sequoiadb_Command/SdbSequence/setCurrentValue.md)

## alterDomain

将该操作应用在集群资源上。

用户可在 SdbDomain 对象上执行以下命令：

- [addGroups](manual/Manual/Sequoiadb_Command/SdbDomain/addGroups.md)
- [alter](manual/Manual/Sequoiadb_Command/SdbDomain/alter.md)
- [setAttributes](manual/Manual/Sequoiadb_Command/SdbDomain/setAttributes.md)
- [removeGroups](manual/Manual/Sequoiadb_Command/SdbDomain/removeGroups.md)
- [setGroups](manual/Manual/Sequoiadb_Command/SdbDomain/setGroups.md)

# 会话操作

## forceSession

将该操作应用在集群资源上。

用户可执行[forceSession](manual/Manual/Sequoiadb_Command/Sdb/forceSession.md)命令。


## trans

将该操作应用在集群资源上。

用户可执行以下命令：

- [transBegin](manual/Manual/Sequoiadb_Command/Sdb/transBegin.md)
- [transRollback](manual/Manual/Sequoiadb_Command/Sdb/transRollback.md)
- [transCommit](manual/Manual/Sequoiadb_Command/Sdb/transCommit.md)

## waitTasks

将该操作应用在集群资源上。

用户可执行[waitTasks](manual/Manual/Sequoiadb_Command/Sdb/waitTasks.md)命令。

# 诊断操作

## analyze

将该操作应用在集合空间或集合资源上。

用户执行[analyze](manual/Manual/Sequoiadb_Command/Sdb/analyze.md)命令时根据参数需要不同的权限：

- 如果参数指定了集合，需要的权限为该集合资源`{ cs: "<cs name>", cl: "<cl name>" }`上的 analyze 操作。
- 如果参数指定了集合空间，需要的权限为该集合空间资源`{ cs: "<cs name>", cl: "" }`上的 analyze 操作。
- 如果参数未指定集合空间和集合，需要的权限为非系统表资源`{ cs: "", cl: "" }`上的 analyze 操作。

## getRole

将该操作应用在集群资源上。

用户可执行[getRole](manual/Manual/Sequoiadb_Command/Sdb/getRole.md)命令。

## getUser

将该操作应用在集群资源上。

用户可执行[getUser](manual/Manual/Sequoiadb_Command/Sdb/getUser.md)命令。

## getDataSource

将该操作应用在集群资源上。

用户可执行如下命令：

- [getDataSource](manual/Manual/Sequoiadb_Command/Sdb/getDataSource.md)
- [listDataSources](manual/Manual/Sequoiadb_Command/Sdb/listDataSources.md)

## getDomain

将该操作应用在集群资源上。

用户可执行如下命令：

- [getDomain](manual/Manual/Sequoiadb_Command/Sdb/getDomain.md)
- [listDomains](manual/Manual/Sequoiadb_Command/Sdb/listDomains.md)

## getSequence

将该操作应用在集群资源上。

用户可执行如下命令：

- [getSequence](manual/Manual/Sequoiadb_Command/Sdb/getSequence.md)
- [listSequences](manual/Manual/Sequoiadb_Command/Sdb/listSequences.md)

## getTask

将该操作应用在集群资源上。

用户可执行如下命令：

- [getTask](manual/Manual/Sequoiadb_Command/Sdb/getTask.md)
- [listTasks](manual/Manual/Sequoiadb_Command/Sdb/listTasks.md)

## listCollections

将该操作应用在集合空间资源上。

用户可在该集合空间对象上执行[listCollections](manual/Manual/Sequoiadb_Command/SdbCS/listCollections.md)命令。

## list

将该操作应用在集群资源上。

用户可执行以下命令:

- [list](manual/Manual/Sequoiadb_Command/Sdb/list.md)
- [listCollections](manual/Manual/Sequoiadb_Command/Sdb/listCollections.md)
- [listCollectionSpaces](manual/Manual/Sequoiadb_Command/Sdb/listCollectionSpaces.md)
- [getRG](manual/Manual/Sequoiadb_Command/Sdb/getRG.md)
- [listGroups](manual/Manual/Sequoiadb_Command/SdbDomain/listGroups.md)
- [getDomain](manual/Manual/Sequoiadb_Command/Sdb/getDomain.md)
- [listDomains](manual/Manual/Sequoiadb_Command/Sdb/listDomains.md)
- [listBackup](manual/Manual/Sequoiadb_Command/Sdb/listBackup.md)
- [getTask](manual/Manual/Sequoiadb_Command/Sdb/getTask.md)
- [listTasks](manual/Manual/Sequoiadb_Command/Sdb/listTasks.md)
- [getSequence](manual/Manual/Sequoiadb_Command/Sdb/getSequence.md)
- [listSequences](manual/Manual/Sequoiadb_Command/Sdb/listSequences.md)
- [getDataSource](manual/Manual/Sequoiadb_Command/Sdb/getDataSource.md)
- [listDataSources](manual/Manual/Sequoiadb_Command/Sdb/listDataSources.md)
- [list (on SdbRecycleBin)](manual/Manual/Sequoiadb_Command/SdbRecycleBin/list.md)
- [listProcedures](manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md)

用户可在 SdbReplicaGroup 对象上执行以下命令：

- [getNode](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getNode.md)
- [getDetailObj](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getDetailObj.md)
- [getMaster](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getMaster.md)
- [getSlave](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getSlave.md)

用户可在 SdbNode 对象上执行以下命令：

- [getDetailObj](manual/Manual/Sequoiadb_Command/SdbNode/getDetailObj.md)

用户可在 SdbCS 对象上执行以下命令：

- [getDomainName](manual/Manual/Sequoiadb_Command/SdbCS/getDomainName.md)

## listCollectionSpaces

将该操作应用在集群资源上。

用户可执行[listCollectionSpaces](manual/Manual/Sequoiadb_Command/Sdb/listCollectionSpaces.md)和list(SDB_LIST_COLLECTIONSPACES)命令。

用户可在 SdbCS 对象上执行以下命令：

- [getDomainName](manual/Manual/Sequoiadb_Command/SdbCS/getDomainName.md)

## snapshot

将该操作应用在集群资源上。

用户可执行以下命令:

- [snapshot](manual/Manual/Sequoiadb_Command/Sdb/snapshot.md)

- [snapshotIndexes](manual/Manual/Sequoiadb_Command/SdbCollection/snapshotIndexes.md)

用户可在 SdbRecycleBin 对象上执行以下命令：

- [snapshot](manual/Manual/Sequoiadb_Command/SdbRecycleBin/snapshot.md)

## setPDLevel

将该操作应用在集群资源上。

用户可执行[setPDLevel](manual/Manual/Sequoiadb_Command/Sdb/setPDLevel.md)命令。

## trace

将该操作应用在集群资源上。

用户可执行以下命令：

- [traceOff](manual/Manual/Sequoiadb_Command/Sdb/traceOff.md)
- [traceOn](manual/Manual/Sequoiadb_Command/Sdb/traceOn.md)
- [traceResume](manual/Manual/Sequoiadb_Command/Sdb/traceResume.md)

## traceStatus

将该操作应用在集群资源上。

用户可执行[traceStatus](manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md)命令。

## listProcedures

将该操作应用在集群资源上。

用户可执行[listProcedures](manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md)命令。

## listBackup

将该操作应用在集群资源上。

用户可执行[listBackup](manual/Manual/Sequoiadb_Command/Sdb/listBackup.md)和list(SDB_LIST_BACKUPS)命令。
