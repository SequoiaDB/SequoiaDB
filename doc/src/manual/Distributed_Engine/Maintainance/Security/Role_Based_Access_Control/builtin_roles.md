
SequoiaDB 预定义了内建角色来提供常用的不同级别的访问权限。内建角色主要分为集合空间级的内建角色和集群级的内建角色。

所有内建角色都不能被删除和修改，但是可以被授予给其他用户自定义角色，也可以被直接授予给用户。

内建角色的命名均以`_`开头，这意味着用户自定义角色不能以`_`开头。

##集合空间级的内建角色##

集合空间级的内建角色主要用于授予用户对集合空间的访问权限。每个集合空间级内建角色的命名规则为`_<cs name>.<role name>`，其中`<cs name>`是集合空间的名称，`<role name>`是角色的名称。角色名称包含以下三种：

###read###
授予对集合空间中所有集合的读权限，通过提供以下操作：
* find
* getDetail
  
###readWrite###
授予对集合空间中所有集合的读写权限，通过提供以下操作：
* find
* getDetail
* insert
* update
* remove
  
###admin###
授予对集合空间中所有集合的读写权限和管理权限，通过提供以下操作：
* find
* getDetail
* insert
* update
* remove
* createIndex
* dropIndex
* copyIndex
* split
* attachCL
* detachCL
* truncate
* alterCL
* testCL
* aggregate
* alterCS
* createCL
* dropCL
* renameCL
* listCollections
* testCS
  
##集群级的内建角色##
集群级的内建角色内建角色主要用于授予用户对集群的访问权限。
###_clusterAdmin###
授予对集群部署和启停的权限，通过提供在集群资源上的以下操作：
* createRG
* removeRG
* startRG
* stopRG
* getRG
* list
* snapshot
* createNode
* removeNode
* startNode
* stopNode
* forceSetUp
  
###_clusterMonitor###
授予对集群的监控权限，通过提供在集群资源上的以下操作：
* list
* snapshot
* resetSnapshot
* getSequenceCurrentValue
* getDCInfo
* getDetailBin
* getRole
* getUser
* countBin
* listCollections
* listBin
* listRoles
* listCollectionSpaces
* listProcedures
* traceStatus
* eval

且提供在所有非系统表资源上的以下操作：
* testCS
* testCL
* getDetail
  
###_backup###
授予集群备份的权限，通过提供在集群资源上的以下操作：
* backup
* listBackup
* removeBackup
  
###_dbAdmin###
授予对所有非系统表的读写、管理权限，通过提供在非系统表上的以下操作：
* find
* getDetail
* insert
* update
* remove
* createIndex
* dropIndex
* copyIndex
* split
* attachCL
* detachCL
* truncate
* alterCL
* testCL
* aggregate
* alterCS
* createCL
* dropCL
* renameCL
* listCollections
* testCS
  
###_userAdmin###
授予对集群中用户和角色管理的权限，通过提供在集群资源上的以下操作：
* createRole
* dropRole
* getRole
* listRoles
* updateRole
* grantPrivilegesToRole
* revokePrivilegesFromRole
* grantRolesToRole
* revokeRolesFromRole
* createUsr
* dropUsr
* getUser
* grantRolesToUser
* revokeRolesFromUser
* invalidateUserCache
  
###_root###
授予对集群中所有资源的所有权限，拥有该角色的用户可以执行任何操作。