SdbReplicaGroup 类主要用于操作复制组和获取复制组相关的信息，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [attachNode()][attachNode] | 将不属于任何复制组的节点加入当前复制组 |
| [createNode()][createNode] | 在当前复制组中创建节点 |
| [detachNode()][detachNode] | 分离当前复制组内的一个节点 |
| [getDetailObj()][getDetailObj] | 获取复制组的信息 |
| [getMaster()][getMaster] | 获取当前复制组的主节点 |
| [getNode()][getNode] | 获取当前复制组的指定节点 |
| [getSlave()][getSlave] | 获取当前复制组的备节点 |
| [reelect()][reelect] | 在当前复制组中重新选举 |
| [removeNode()][removeNode] | 删除当前复制组中的指定节点 |
| [setActiveLocation()][setActiveLocation] | 设置复制组的 ActivedLocation |
| [setAttributes()][setAttributes] | 修改当前复制组的属性 |
| [start()][start] | 启动当前复制组 |
| [startCriticalMode()][startCriticalMode] | 在当前复制组中开启 Critical 模式 |
| [startMaintenanceMode()][startMaintenanceMode] | 在当前复制组中开启 Maintenance 模式 |
| [stop()][stop] | 停止当前复制组 |
| [stopCriticalMode()][stopCriticalMode] | 在当前复制组中停止 Critical 模式 |
| [stopMaintenanceMode()][stopMaintenanceMode] | 在当前复制组中停止 Maintenance 模式 |

[^_^]:
     本文使用的所有引用及链接
[attachNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/attachNode.md
[createNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md
[detachNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/detachNode.md
[getDetailObj]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getDetailObj.md
[getMaster]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getMaster.md
[getNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getNode.md
[getSlave]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/getSlave.md
[reelect]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/reelect.md
[removeNode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/removeNode.md
[setActiveLocation]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/setActiveLocation.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/setAttributes.md
[start]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md
[startCriticalMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startCriticalMode.md
[startMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startMaintenanceMode.md
[stop]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stop.md
[stopCriticalMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stopCriticalMode.md
[stopMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stopCriticalMode.md