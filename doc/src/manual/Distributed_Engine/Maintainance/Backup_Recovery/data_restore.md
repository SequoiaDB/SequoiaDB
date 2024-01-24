SequoiaDB 巨杉数据库支持将备份的数据恢复至集群节点或离线库中。

## 数据恢复工具

用户可使用 sdbrestore 工具对目标节点进行数据恢复。使用该工具进行数据恢复时，需要确保备份文件的所属用户为数据库管理用户（安装 SequoiaDB 时创建，默认为 sdbadmin）。

sdbrestore 工具的参数分为功能参数和配置参数。当用户需要恢复备份源节点的数据时，指定功能参数即可；当用户将一份数据恢复至不同的节点或构建离线库时，需要功能参数和配置参数同时指定。

### 功能参数

sdbrestore 工具的功能参数可用于配置需要恢复的数据范围、恢复行为等。

| 参数名        | 缩写 | 说明 |
| ------------- | ---- | ---- |
| --bkpath      | -p   | 备份源数据所在路径  |
| --beginincreaseid | -b | 需要从第几次备份开始恢复，默认为 -1，表示由恢复工具自动计算，恢复前确保当前恢复节点的 expectLSN 位于备份文件的 beginLSN 和 endLSN 范围内 <br> 为 0 时，表示从全量备份开始恢复 <br> 为 1 时，表示从第一次增量备份开始恢复，以此类推 <br> 可参考 [listBackup()][listBackup] 输出的字段值 ID，选定需要的备份；如选定 ID 为 2 的备份，则 -b 指定为 2 |
| --increaseid  | -i   | 需要恢复到第几次增量备份，默认为 -1，表示恢复到最后一次 <br> 取值方式可参考参数 --beginincreaseid  |
| --bkname      | -n   | 需要恢复的备份名称 |
| --action      | -a   | 恢复行为，默认为"restore"，取值如下：<br>"restore"：恢复<br>"list"：查看备份信息   |
| --diaglevel   | -v   | 恢复工具自身的日志级别，默认为 3，表示 WARNING，具体取值可参考[配置项参数][configuration] |
| --isSelf      |      | 是否将数据恢复至备份源节点，默认为 true，恢复至备份源节点 |

### 配置参数

sdbrestore 工具的配置参数可用于配置备份文件的相关恢复路径，用户可根据实际情况选择性配置。如果不指定配置参数，则所有恢复路径为节点配置文件中定义的路径；如果指定了配置参数，则相关恢复路径将使用指定的路径，且指定的配置参数会覆盖配置文件中对应的配置项。

> **Note:**
>
> 当 --isSelf 设置为 false 时，用户必须配置 dbpath、confpath 和 svcname 参数，否则执行报错。

| 参数名       | 说明          |
| ------------ | ------------- |
| --dbpath     | 目标节点的数据文件目录  |
| --confpath   | 目标节点的配置文件路径 |
| --svcname    | 目标节点的服务名或端口 |
| --indexpath  | 目标节点的索引文件目录 |
| --logpath    | 目标节点的日志文件目录 |
| --diagpath   | 目标节点的诊断日志文件目录 |
| --auditpath  | 目标节点的审记日志文件目录 |
| --bkuppath   | 目标节点的备份文件目录 |
| --archivepath | 目标节点的日志归档目录 |
| --lobmetapath | 目标节点的大对象元数据文件目录 |
| --lobpath     | 目标节点的大对象数据文件目录 |
| --replname    | 目标节点的复制通讯服务名或端口 |
| --shardname   | 目标节点的分区通讯服务名或端口 |
| --catalogname | 目标节点的编目通讯服务名或端口 |
| --httpname    | 目标节点的 REST 服务名或端口 |


## 数据恢复

用户通过 sdbrestore 工具恢复当前集群中的节点时，需要先停止运行目标节点；如果需要恢复目标节点所在复制组的数据，则需要先停止该复制组。恢复过程中，sdbrestore 工具会清空目标节点的所有数据和日志，再从备份的数据中恢复配置、数据和日志。

### 恢复步骤

1. 启动 SDB Shell，并连接至协调节点

    ```lang-javascript
    > var db = new Sdb("localhost",11810)
    ```

2. 停止被恢复节点所在复制组

    ```lang-javascript
    > db.stopRG("group1")
    ```

3. 以恢复 11820 节点的数据为例，备份源数据所在路径为 `/opt/sequoiadb/database/data/11820/bakfile`，需要恢复的备份名为“backupAll_group1”

    ```lang-bash
    $ sdbrestore -p /opt/sequoiadb/database/data/11820/bakfile -n backupAll_group1
    ```

    输出如下结果表示数据恢复成功：

    ```lang-text
    Check sequoiadb(11820) is not running...OK
    Begin to init dps logs...
    Begin to restore... 
    Begin to restore data file: /opt/sequoiadb/database/data/11820/bakfile/backupAll_group1.1 ...
    Begin to restore su: SYSSTAT.1.data ...
    Begin to restore su: SYSSTAT.1.idx ...
    Begin to restore su: SYSLOCAL.1.data ...
    Begin to restore su: SYSLOCAL.1.idx ...
    Begin to wait repl bucket empty...
    *****************************************************
    Restore succeed!
    *****************************************************
    ```

4. 恢复同组内其他节点数据

    用户可通过 sdbrestore 工具或文件拷贝的方式，恢复同组内其他节点的数据。

    - 通过 sdbrestore 工具恢复

        将 11820 节点的数据恢复至同组的 11850 节点中，需要指定 --isSelf 为 false 及配置相关参数

        ```lang-bash
        $ sdbrestore -p /opt/sequoiadb/database/data/11820/bakfile -n backupAll_group1 --isSelf false --dbpath /opt/sequoiadb/database/data/11850 --confpath /opt/sequoiadb/conf/local/11850/ --svcname 11850
        ```

    - 通过文件拷贝

        用户可以将源节点的 .data 文件、.idx 文件、.lobd 文件、.lobm 文件和 replicalog 日志，拷贝至同组的其他数据节点相应目录下，实现数据恢复。

5. 进入 11850 节点的数据目录下查看是否存在新的数据文件

    ```lang-bash
    $ ll /opt/sequoiadb/database/data/11850
    ```

    输出结果如下，存在新的数据文件则表示恢复成功：

    ```lang-text
    drwxr-xr-x 2 sdbadmin sdbadmin_group      4096 1月  15 16:20 replicalog/
    -rw-r----- 1 sdbadmin sdbadmin_group 155254784 1月  18 13:44 sample.1.data
    -rw-r----- 1 sdbadmin sdbadmin_group 151060480 1月  18 13:44 sample.1.idx
    -rw-r----- 1 sdbadmin sdbadmin_group  21037056 1月  18 13:44 SYSLOCAL.1.data
    -rw-r----- 1 sdbadmin sdbadmin_group  50397184 1月  18 13:44 SYSLOCAL.1.idx
    -rw-r----- 1 sdbadmin sdbadmin_group  21037056 1月  18 13:44 SYSSTAT.1.data
    -rw-r----- 1 sdbadmin sdbadmin_group  50397184 1月  18 13:44 SYSSTAT.1.idx
    ```

## 构建离线库

离线库用于存储离线数据。sdbrestore 工具可以将节点全量备份和增量备份的数据，不断合并成一份与节点内数据完全相同的离线数据。用户可以将离线数据存储于离线库中，便于节点故障后，通过离线数据实现快速恢复。

### 构建步骤

1. 生成离线数据前需要先创建离线库所在目录，且该目录所属用户为数据库管理用户。

2. 使用 11820 节点的备份数据构建该节点的离线库，需要指定 --isSelf false 及相关配置参数，离线库所在路径为  `/opt/backup/11820`

    ```lang-bash
    $ sdbrestore -p /opt/sequoiadb/database/data/11820/bakfile -n backupAll_group1 --isSelf false --dbpath /opt/backup/11820 --confpath /opt/sequoiadb/conf/local/11820/ --svcname 11820 
    ```

### 使用

当节点 11820 或同组备节点发生故障时，用户可将离线数据直接拷贝至节点 11820 或同组节点的数据文件目录下，以实现数据的快速恢复。



[^_^]:
    本文使用的所有引用及链接
[listBackup]:manual/Manual/Sequoiadb_Command/Sdb/listBackup.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md