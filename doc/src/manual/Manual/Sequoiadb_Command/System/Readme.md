System 类主要用于获取和操作系统的属性数据，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [addAHostMap][addAHostMap] | 往 host 文件添加一条主机名到 IP 地址的映射关系 |
| [addGroup][addGroup] | 添加用户组 |
| [addUser][addUser] | 新增操作系统用户 |
| [delAHostMap][delAHostMap] | 删除 host 文件中的一条指定主机名的映射关系 |
| [delGroup][delGroup] | 删除系统用户组 |
| [delUser][delUser] | 删除操作系统用户 |
| [getAHostMap][getAHostMap] | 获取指定主机名在 host 文件中对应的 IP 地址 |
| [getCpuInfo][getCpuInfo] | 获取 CPU 的信息 |
| [getCurrentUser][getCurrentUser] | 获取当前用户信息 |
| [getDiskInfo][getDiskInfo] | 获取磁盘的信息 |
| [getEWD][getEWD] | 获取当前 sdb shell 所在的目录 |
| [getHostName][getHostName] | 获取主机名 |
| [getHostsMap][getHostsMap] | 获取 host 文件的 IP 与主机名的映射关系 |
| [getIpTablesInfo][getIpTablesInfo] | 获取防火墙信息 |
| [getMemInfo][getMemInfo] | 获取内存信息 |
| [getNetcardInfo][getNetcardInfo] | 获取网卡的信息 |
| [getPID][getPID] | 获取运行 sdb shell 的进程 ID |
| [getProcUlimitConfigs][getProcUlimitConfigs] | 获取进程资源限制值|
| [getReleaseInfo][getReleaseInfo] | 获取操作系统发行版本信息 |
| [getSystemConfigs][getSystemConfigs] | 获取系统配置信息 |
| [getTID][getTID] | 获取运行 sdb shell 的线程 ID |
| [getUserEnv][getUserEnv] | 获取当前用户的环境变量 |
| [isGroupExist][isGroupExist] | 判断指定用户组是否存在 |
| [isProcExist][isProcExist] | 判断指定进程是否存在 |
| [isUserExist][isUserExist] | 判断指定用户是否存在 |
| [killProcess][killProcess] | 杀死指定进程 |
| [listAllUsers][listAllUsers] | 列出用户的信息 |
| [listGroups][listGroups] | 列出用户组的信息 |
| [listLoginUsers][listLoginUsers] | 列出登录用户的信息 |
| [listProcess][listProcess] | 列出进程的信息 |
| [ping][ping] | 判断到达指定主机的网络是否连通 |
| [runService][runService] | 运行 service 命令 |
| [setProcUlimitConfigs][setProcUlimitConfigs] | 修改进程资源限制值 |
| [setUserConfigs][setUserConfigs] | 修改操作系统用户的配置 |
| [snapshotCpuInfo][snapshotCpuInfo] | 获取 CPU 的基本信息 |
| [snapshotDiskInfo][snapshotDiskInfo] | 获取磁盘的信息 |
| [snapshotMemInfo][snapshotMemInfo] | 获取内存的基本信息 |
| [snapshotNetcardInfo][snapshotNetcardInfo] | 获取网卡的详细信息 |
| [sniffPort][sniffPort] | 判断指定端口是否可用 | 
| [type][type] | 获取操作系统类别 |

[^_^]:
     本文使用的所有引用及链接
[addAHostMap]:manual/Manual/Sequoiadb_Command/System/addAHostMap.md
[addGroup]:manual/Manual/Sequoiadb_Command/System/addGroup.md
[addUser]:manual/Manual/Sequoiadb_Command/System/addUser.md
[delAHostMap]:manual/Manual/Sequoiadb_Command/System/delAHostMap.md
[delGroup]:manual/Manual/Sequoiadb_Command/System/delGroup.md
[delUser]:manual/Manual/Sequoiadb_Command/System/delUser.md
[getAHostMap]:manual/Manual/Sequoiadb_Command/System/getAHostMap.md
[getCpuInfo]:manual/Manual/Sequoiadb_Command/System/getCpuInfo.md
[getCurrentUser]:manual/Manual/Sequoiadb_Command/System/getCurrentUser.md
[getDiskInfo]:manual/Manual/Sequoiadb_Command/System/getDiskInfo.md
[getEWD]:manual/Manual/Sequoiadb_Command/System/getEWD.md
[getHostName]:manual/Manual/Sequoiadb_Command/System/getHostName.md
[getHostsMap]:manual/Manual/Sequoiadb_Command/System/getHostsMap.md
[getIpTablesInfo]:manual/Manual/Sequoiadb_Command/System/getIpTablesInfo.md
[getMemInfo]:manual/Manual/Sequoiadb_Command/System/getMemInfo.md
[getNetcardInfo]:manual/Manual/Sequoiadb_Command/System/getNetcardInfo.md
[getPID]:manual/Manual/Sequoiadb_Command/System/getPID.md
[getProcUlimitConfigs]:manual/Manual/Sequoiadb_Command/System/getProcUlimitConfigs.md
[getReleaseInfo]:manual/Manual/Sequoiadb_Command/System/getReleaseInfo.md
[getSystemConfigs]:manual/Manual/Sequoiadb_Command/System/getSystemConfigs.md
[getTID]:manual/Manual/Sequoiadb_Command/System/getTID.md
[getUserEnv]:manual/Manual/Sequoiadb_Command/System/getUserEnv.md
[isGroupExist]:manual/Manual/Sequoiadb_Command/System/isGroupExist.md
[isProcExist]:manual/Manual/Sequoiadb_Command/System/isProcExist.md
[isUserExist]:manual/Manual/Sequoiadb_Command/System/isUserExist.md
[killProcess]:manual/Manual/Sequoiadb_Command/System/killProcess.md
[listAllUsers]:manual/Manual/Sequoiadb_Command/System/listAllUsers.md
[listGroups]:manual/Manual/Sequoiadb_Command/System/listGroups.md
[listLoginUsers]:manual/Manual/Sequoiadb_Command/System/listLoginUsers.md
[listProcess]:manual/Manual/Sequoiadb_Command/System/listProcess.md
[ping]:manual/Manual/Sequoiadb_Command/System/ping.md
[runService]:manual/Manual/Sequoiadb_Command/System/runService.md
[setProcUlimitConfigs]:manual/Manual/Sequoiadb_Command/System/setProcUlimitConfigs.md
[setUserConfigs]:manual/Manual/Sequoiadb_Command/System/setUserConfigs.md
[snapshotCpuInfo]:manual/Manual/Sequoiadb_Command/System/snapshotCpuInfo.md
[snapshotDiskInfo]:manual/Manual/Sequoiadb_Command/System/snapshotDiskInfo.md
[snapshotMemInfo]:manual/Manual/Sequoiadb_Command/System/snapshotMemInfo.md
[snapshotNetcardInfo]:manual/Manual/Sequoiadb_Command/System/snapshotNetcardInfo.md
[sniffPort]:manual/Manual/Sequoiadb_Command/System/sniffPort.md
[type]:manual/Manual/Sequoiadb_Command/System/type.md