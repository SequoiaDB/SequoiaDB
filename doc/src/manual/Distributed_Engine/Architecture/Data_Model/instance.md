在 SequoiaDB 巨杉数据库中，实例（Instance）是包含完整集合记录的数据集。在多副本环境下，如果同一个域中存在多份完整的集合记录，即存在多个实例。在业务场景中，用户可以设置不同业务访问不同的实例，以实现资源的隔离。

##逻辑架构##

实例间通过实例 ID（instanceid）进行标识，节点的 instanceid 默认根据复制组中 NodeID 的正序顺序，从 1 开始自动分配。其架构图如下：

![实例逻辑图][instance]

##实例划分规则##

用户可手动配置节点的 instanceid，具体操作可参考 [updateConf()][updateConf]。自行配置 instanceid 后，用户需保证 instanceid 相同的节点可以构成一份完整的集合记录，如图 1 所示；如果 instanceid 相同的节点未包含完整的集合记录，则不能称为一个实例，如图 2 所示。

![示例][instance_error]

[^_^]:
    本文使用的所有引用及链接
[instance]:images/Distributed_Engine/Architecture/Data_Model/instance.png
[instance_error]:images/Distributed_Engine/Architecture/Data_Model/instance_error.png
[updateConf]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
[configeration]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md