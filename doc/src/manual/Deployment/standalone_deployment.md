[^_^]:
    独立模式部署
    作者：秦志强
    时间：
    评审意见
    王涛：
    许建辉：时间：
    市场部：时间：20190321


本文档主要介绍在本地主机部署 SequoiaDB 巨杉数据库的独立模式。

##独立模式说明##

独立模式是 SequoiaDB 部署的最简模式，仅需要启动一个独立模式的[数据节点][data_node]，即可提供数据服务。该模式中所有信息都存放在数据节点，不存在编目信息。

独立模式仅支持通过 JSON API 操作数据库，不建议在生产环境中使用。如果需要部署 SQL 实例，或使用高可用与容灾、数据分区、数据复制等能力，可参考[集群模式][cluster_deployment]。

##检查服务状态##

在进行独立模式部署前，用户需要在数据库服务器上检查 SequoiaDB 配置服务的状态。

```lang-bash
# service sdbcm status
```

系统提示“sdbcm is running”表示服务正在运行，否则执行如下指令重新配置服务程序：

```lang-bash
# service sdbcm start
```

##部署##

下述操作步骤假设 SequoiaDB 程序安装在 `/opt/sequoiadb` 目录下。
> **Note：**
>
> SequoiaDB 服务进程全部以 sdbadmin 用户运行，用户应确保数据库目录都赋予 sdbadmin 读写权限。

1. 切换到 sdbadmin 用户

    ```lang-bash
    $ su - sdbadmin
    ```

2. 进入 SDB Shell 控制台

    ```lang-bash
    $ /opt/sequoiadb/bin/sdb
    ```

3. 连接本地集群管理服务进程 sdbcm

    ```lang-bash
    > var oma = new Oma("localhost", 11790)
    ```

4. 创建数据节点

    ```lang-bash
    > oma.createData(11810, "/opt/sequoiadb/database/standalone/11810")
    ```

    > **Note:**
    >
    > 其中 11810 为数据库服务端口名，为避免端口冲突，可将数据库端口配置在随机端口范围之外。如：多数 Linux 默认随机端口范围为 32768～61000，可将数据库端口配置在 32767 以下。  

5. 启动该节点

    ```lang-bash
    > oma.startNode(11810)
    ```

至此，SequoiaDB 独立模式已部署完毕，用户可通过 SDB Shell 进行数据操作。



[^_^]:
     本文使用的所有引用及链接
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[cluster_deployment]:manual/Deployment/cluster_deployment.md

