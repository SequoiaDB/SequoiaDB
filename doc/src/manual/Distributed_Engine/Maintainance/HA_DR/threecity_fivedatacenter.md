[^_^]:
    三地五中心部署

本文档主要介绍在三地五中心的部署方案下，如何应对不同级别的灾难。

##灾难应对方案##

###节点故障###

当复制组中超过半数节点发生故障，该复制组将无法提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。如果故障节点未超过半数，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障节点开启运维模式，修复并恢复节点数据即可。

![节点故障情况][3c5d_node_down]

###单个数据中心故障###

当单个数据中心发生故障，集群仍对外提供读写服务。针对该情况，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障中心的节点开启运维模式，修复故障中心并恢复节点数据即可。

![单个数据中心故障情况][3c5d_singlecenter_down]

###城市级中心故障###

当一个城市发生灾难，导致两个数据中心故障，另外两个城市的数据中心仍可提供读写服务。针对该情况，用户可通过 [startMaintenanceMode()][startMaintenanceMode] 命令对故障中心的节点开启运维模式，修复故障中心并恢复节点数据即可。

![单个城市灾难情况][3c5d_singcity_down]

当两个城市发生灾难，集群将失去半数以上的节点，导致无法对外提供读写服务。针对该情况，用户需进行[灾难恢复][recovery]。

![两个城市灾难情况][3c5d_twocity_down]

##灾难恢复##

在进行灾难恢复时，用户需根据实际情况选取待恢复的主机，并在该主机上执行后续恢复步骤，实现业务的接管。待恢复主机的选取规则如下：

- 在主中心未故障的情况下，优先选取主中心的机器作为待恢复主机。
- 在主中心故障且存在多个可用灾备中心的情况下，优先选取与主中心具有[亲和性][location_principle]的灾备中心。该灾备中心的任意机器均可作为待恢复主机。

下述以 SequoiaDB 安装目录 `/opt/sequoiadb/`、编目节点 11800、协调节点 11810、集群鉴权用户名“sdbadmin”和用户密码“sdbadmin”为例，介绍灾难恢复步骤。

###恢复编目复制组###

如果集群因数据中心整体故障而导致无法对外提供服务，用户需恢复编目复制组。数据节点故障和网络故障场景可跳过此步骤。

1. 关闭鉴权功能

    ```lang-bash
    $ echo "auth=false" >> /opt/sequoiadb/conf/local/11800/sdb.conf
    ```

2. 重启编目节点，使配置生效

    ```lang-bash
    $ sdbstop -p 11800
    $ sdbstart -p 11800
    ```

3. 通过 SDB Shell 将当前主机的编目节点升主

    ```lang-javascript
    > var cata = new Sdb("localhost", 11800)
    > cata.forceStepUp()
    ```

4. 在编目复制组中开启 Critical 模式

    ```lang-javascript
    > var db = new Sdb("localhost", 11810)
    > var cataRG = db.getRG("SYSCatalogGroup")
    > cataRG.startCriticalMode({Location: "Guangzhou.Panyu", MinKeepTime: 100, MaxKeepTime: 1000})
    ```

###恢复数据复制组###

1. 在各数据复制组中开启 Critical 模式

    ```lang-javascript
    > var dataRG = db.getRG("group1")
    > dataRG.startCriticalMode({Location: "Guangzhou.Panyu", MinKeepTime: 100, MaxKeepTime: 1000})
    ```

    > **Note:**
    >
    > 参数 MaxKeepTime 表示 Critical 模式的最高运行窗口时间。如果在该时间内故障未修复，系统将强制解除 Critical 模式，集群将回到不可用状态。因此用户需根据实际的故障修复耗时，指定该参数的取值，避免多次执行开启操作。

2. 查看是否成功开启

    ```lang-javascript
    > db.list(SDB_LIST_GROUPMODES)
    ```

    输出结果如下，字段 GroupMode 显示为 critical 表示开启成功：

    ```lang-json
    {
      "_id": {
        "$oid": "6458b62bdfc87b1c4344e16b"
      },
      "GroupID": 1,
      "GroupMode": "critical",
      "Properties": [
        {
          "Location": " Guangzhou.Panyu",
          "MinKeepTime": "2023-05-08-18.23.23.445185",
          "MaxKeepTime": "2023-05-09-09.23.23.445185",
          "UpdateTime": "2023-05-08-16.43.23.445185"
        }
      ]
    }
    ···
    ```

###恢复故障节点###

1. 逐一开启故障节点的自动全量同步功能

    ```lang-bash
    $ sed -i "s/dataerrorop=.*/dataerrorop=1/g" /opt/sequoiadb/conf/local/<端口号>/sdb.conf
    ```

2. 修复故障

3. 逐一启动故障数据节点

    ```lang-bash
    $ sdbstart -p <故障节点端口号>
    ```

4. 通过 sdblist 检查各故障数据节点的 GroupID(GID) 和 NodeID(NID) 是否生成，确保故障数据节点注册成功

    ```lang-bash
    $ sdblist -l
    ```

5. 逐一启动故障编目节点

    ```lang-bash
    $ sdbstart -p <故障节点端口号>
    ```

6. 检查各主机的节点是否成功启动

    ```lang-bash
    $ sdblist -l
    ```

7. 检查节点健康检测快照，确定各节点状态恢复为 Normal

###恢复集群配置###

1. 通过 SDB Shell 检查 Critical 模式是否解除

    ```lang-javascript
    > db.list(SDB_LIST_GROUPMODES)
    ```

    如果字段 GroupMode 显示为 critical 表示未解除，需执行如下命令手动解除：

    ```lang-javascript
    > var dataRG = db.getRG("group1")
    > dataRG.stopCriticalMode()
    > var cataRG = db.getRG("SYSCatalogGroup")
    > cataRG.stopCriticalMode()
    ```

    > **Note:**
    >
    > 手动解除 Critical 模式前请确保集群已恢复，否则集群将回到不可用状态。

2. 重新选举各复制组中的主节点，使主节点恢复至故障前所在的数据中心

    ```lang-javascript
    > dataRG.reelect({Seconds: 60})
    > cataRG.reelect({Seconds: 60})
    ```

3. 关闭非 ActiveLocation 位置集下，所有节点的自动全量同步功能

    ```lang-javascript
    > db.updateConf({"dataerrorop": 2}, {HostName: ["sdbserver2", "sdbserver3", "sdbserver4", "sdbserver5"]})
    ```

4. 通过命令行检查鉴权功能的状态

    ```lang-bash
    $ cat /opt/sequoiadb/conf/local/11800/sdb.conf
    ```

    如果参数 auth 的取值为 false ，表示鉴权功能为关闭状态，用户需手动执行如下命令开启鉴权：

    ```lang-bash
    $ sed -i "s/auth=.*/auth=true/g" /opt/sequoiadb/conf/local/11800/sdb.conf
    ```

5. 重启编目节点，使配置生效

    ```lang-bash
    $ sdbstop -p 11800
    $ sdbstart -p 11800
    ```

[^_^]:
    本文使用到的所有链接
[split_merge]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md
[consistency]:manual/Distributed_Engine/Architecture/Replication/primary_secondary_consistency.md
[threecity_fivedatacenter_usage]:manual/Distributed_Engine/Maintainance/HA_DR/disaster_recovery_tool.md
[recovery]:manual/Distributed_Engine/Maintainance/HA_DR/threecity_fivedatacenter.md#灾难恢复
[3c5d_node_down]:images/Distributed_Engine/Maintainance/HA_DR/3c5d_node_down.png
[3c5d_singlecenter_down]:images/Distributed_Engine/Maintainance/HA_DR/3c5d_singlecenter_down.png
[3c5d_singcity_down]:images/Distributed_Engine/Maintainance/HA_DR/3c5d_singcity_down.png
[3c5d_twocity_down]:images/Distributed_Engine/Maintainance/HA_DR/3c5d_twocity_down.png
[location_principle]:manual/Distributed_Engine/Architecture/Location/location_principle.md#位置亲和性
[startMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/startMaintenanceMode.md