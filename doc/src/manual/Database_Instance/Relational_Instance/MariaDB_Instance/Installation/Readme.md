[^_^]:
    MariaDB 实例-安装 Readme

MariaDB 实例组件支持部署单实例或实例组模式。用户在[安装 MariaDB 实例组件][install_deploy1]后，可以根据实际情况选择部署模式。

- **单实例模式**

    在单实例模式下，各实例的元数据只存储在该实例本身，实例间不可进行跨实例的数据操作。用户可通过简单地[部署 MariaDB 实例组件][install_deploy2]实现单实例模式部署。

- **实例组模式**

    在[实例组][instance_group]模式下，MariaDB 实例组件为应用提供统一的元数据视图，并保证集群的元数据一致性。同时，当一个 MariaDB 实例退出实例组后，连接该实例的应用可以切换到实例组内的其他实例，获得对等的读写服务，以保证服务的高可用。

通过本章文档，用户可以了解 MariaDB 实例组件的安装及部署。主要内容如下：

- [安装部署][install_deploy]
- [实例组][instance_group]


[^_^]:
     本文使用的所有引用及链接
[install_deploy1]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/install_deploy.md#安装%20MariaDB%20实例组件
[install_deploy2]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/install_deploy.md#部署%20MariaDB%20实例组件
[instance_group]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/instance_group.md
[install_deploy]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Installation/install_deploy.md
