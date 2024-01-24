Oma 类主要用于集群管理，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [Oma()][Oma] | 集群管理对象 |
| [addAOmaSvcName()][addAOmaSvcName] | 将目标机器为 sdbcm 设置的服务端口号写到该 sdbcm 的配置文件中 |
| [delAOmaSvcName()][delAOmaSvcName] | 将目标机器 sdbcm 的服务端口号从其配置文件中删除 |
| [close()][close] | 关闭 Oma 连接对象 |
| [createCoord()][createCoord] | 在目标集群控制器（sdbcm）所在的机器中创建一个 coord 节点 |
| [createData()][createData]| 在目标集群控制器（sdbcm）所在的机器中创建一个 standalone 节点 |
| [createOM()][createOM] | 在目标集群控制器（sdbcm）所在的机器中创建 sdbom 服务进程（ SequoiaDB 管理中心进程） |
| [getAOmaSvcName()][getAOmaSvcName] | 获取目标机器 sdbcm 的服务端口 |
| [getIniConfigs()][getIniConfigs] | 获取 INI 文件的配置信息 |
| [getNodeConfigs()][getNodeConfigs] | 从配置文件中获取指定端口的数据库节点的配置信息 |
| [getOmaConfigFile()][getOmaConfigFile] | 获取 sdbcm 的配置文件 |
| [getOmaConfigs()][getOmaConfigs] | 获取 sdbcm 的配置信息 |
| [getOmaInstallFile()][getOmaInstallFile] | 获取安装信息文件 |
| [getOmaInstallInfo()][getOmaInstallInfo] | 从安装信息文件中获取安装信息 |
| [listNodes()][listNodes] | 列出当前所连 sdbcm 所在机器符合条件的所有节点的信息 |
| [reloadConfigs()][reloadConfigs] | sdbcm 重新加载其配置文件的内容，并使其生效 |
| [removeCoord()][removeCoord] | 在目标集群控制器（sdbcm）所在的机器中删除一个 coord 节点 |
| [removeData()][removeData] | 在目标集群控制器（sdbcm）所在的机器中删除指定的 standalone 节点 |
| [removeOM()][removeOM] | 在目标集群控制器（sdbcm）所在的机器中删除 sdbom 服务进程（SequoiaDB 管理中心进程） |
| [setIniConfigs()][setIniConfigs] | 把配置信息写入 INI 文件 |
| [setNodeConfigs()][setNodeConfigs] | 对指定端口的数据库节点，用新的节点配置信息覆盖该节点原来配置文件上的配置信息 |
| [setOmaConfigs()][setOmaConfigs] | 把 sdbcm 的配置信息写入到其配置文件 |
| [start()][start] | 启动 sdbcm 服务 |
| [startAllNodes()][startAllNodes] | 在目标集群控制器（sdbcm）所在的机器中启动所有属于指定业务的节点 |
| [stopAllNodes()][stopAllNodes] | 在目标集群控制器（sdbcm）所在的机器中停止所有属于指定业务的节点 |
| [startNode()][startNode] | 在目标集群控制器（sdbcm）所在的机器中启动指定节点 |
| [stopNode()][stopNode] | 在目标集群控制器（sdbcm）所在的机器中停止指定节点 |
| [startNodes()][startNodes] | 通过服务端口启动节点 |
| [stopNodes()][stopNodes] | 在目标集群控制器（sdbcm）所在的机器中停止指定节点 |
| [updateNodeConfigs()][updateNodeConfigs] | 对指定端口的数据库节点，用新的节点配置信息更新该节点原来配置文件上的配置信息 |

[^_^]:
     本文使用的所有引用及链接
[Oma]:manual/Manual/Sequoiadb_Command/Oma/Oma.md
[addAOmaSvcName]:manual/Manual/Sequoiadb_Command/Oma/addAOmaSvcName.md
[delAOmaSvcName]:manual/Manual/Sequoiadb_Command/Oma/delAOmaSvcName.md
[close]:manual/Manual/Sequoiadb_Command/Oma/close.md
[createCoord]:manual/Manual/Sequoiadb_Command/Oma/createCoord.md
[createData]:manual/Manual/Sequoiadb_Command/Oma/createData.md
[createOM]:manual/Manual/Sequoiadb_Command/Oma/createOM.md
[getAOmaSvcName]:manual/Manual/Sequoiadb_Command/Oma/getAOmaSvcName.md
[getIniConfigs]:manual/Manual/Sequoiadb_Command/Oma/getIniConfigs.md
[getNodeConfigs]:manual/Manual/Sequoiadb_Command/Oma/getNodeConfigs.md
[getOmaConfigFile]:manual/Manual/Sequoiadb_Command/Oma/getOmaConfigFile.md
[getOmaConfigs]:manual/Manual/Sequoiadb_Command/Oma/getOmaConfigs.md
[getOmaInstallFile]:manual/Manual/Sequoiadb_Command/Oma/getOmaInstallFile.md
[getOmaInstallInfo]:manual/Manual/Sequoiadb_Command/Oma/getOmaInstallInfo.md
[listNodes]:manual/Manual/Sequoiadb_Command/Oma/listNodes.md
[reloadConfigs]:manual/Manual/Sequoiadb_Command/Oma/reloadConfigs.md
[removeCoord]:manual/Manual/Sequoiadb_Command/Oma/removeCoord.md
[removeData]:manual/Manual/Sequoiadb_Command/Oma/removeData.md
[removeOM]:manual/Manual/Sequoiadb_Command/Oma/removeOM.md
[setIniConfigs]:manual/Manual/Sequoiadb_Command/Oma/setIniConfigs.md
[setNodeConfigs]:manual/Manual/Sequoiadb_Command/Oma/setNodeConfigs.md
[setOmaConfigs]:manual/Manual/Sequoiadb_Command/Oma/setOmaConfigs.md
[start]:manual/Manual/Sequoiadb_Command/Oma/start.md
[startAllNodes]:manual/Manual/Sequoiadb_Command/Oma/startAllNodes.md
[stopAllNodes]:manual/Manual/Sequoiadb_Command/Oma/stopAllNodes.md
[startNode]:manual/Manual/Sequoiadb_Command/Oma/startNode.md
[stopNode]:manual/Manual/Sequoiadb_Command/Oma/stopNode.md
[startNodes]:manual/Manual/Sequoiadb_Command/Oma/startNodes.md
[stopNodes]:manual/Manual/Sequoiadb_Command/Oma/stopNodes.md
[updateNodeConfigs]:manual/Manual/Sequoiadb_Command/Oma/updateNodeConfigs.md