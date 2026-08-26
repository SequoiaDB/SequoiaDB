sdb-schedule 是一个基于微服务架构实现的 SequoiaDB 数据迁移与切换工具，一套 sdb-schedule 集群可以部署在多台服务器上，集群内选出一个节点作为主节点，负责调度任务的管理及任务分发，其他节点作为从节点，负责执行主节点分发的任务。

![系统架构图][system_architecture.png]

## 数据存储

元数据服务 （Meta Source） 主要负责存储整个系统的元数据信息。目前由 rootsite 站点提供元数据服务的能力。

## 业务概念

### 站点

在 sdb-schedule 中以站点的形式区分不同的 SequoiaDB 集群。每个站点对应一个 SequoiaDB 集群，因此每个需要进行数据迁移的 SequoiaDB 集群都需要在 sdb-schedule 中创建一个站点。

注意：rootsite 站点同时提供元数据服务。

### 调度服务

sdb-schedule 本质上由调度服务构成，调度服务主要提供调度任务的相关功能，负责周期调度和执行迁移任务、数据切换任务和清理任务。调度任务的调度周期通过 cron 表达式指定，利用 cron 表达式，调度服务支持丰富的调度规则。

#### 节点选举

调度服务节点的选举通过往共享存储（SequoiaDB）定期续约的机制来实现。

- 主节点承担调度服务的主体工作，包括增删改查调度任务、创建后台任务及通知节点执行任务等

- 从节点对外仅承担相关业务的查询请求，同时也负责后台任务的执行。在主节点掉线时，从节点能升主接管主节点的工作

#### 调度任务的创建与执行

- web 页面将调度任务的创建信息发给本节点，本节点转发请求至调度服务主节点

- 调度服务主节点将新建的任务持久至元数据服务，并根据任务信息，在节点内部启动定时器，定时触发新建的调度任务

- 每当调度服务主节点的定时器触发时，主节点将对应的任务信息提交至负责执行的调度服务节点，由其执行具体的任务内容

#### 迁移调度任务

迁移调度任务支持周期调度迁移源站点上的集合数据到目标站点上，支持增量迁移 LOB 数据；不支持增量迁移记录数据，每次变更记录数据，都将全量迁移记录数据：

![迁移示意图][transfer.png]

- 由用户指定迁移的源站点和目标站点

- 支持设定要迁移的集合空间/集合（支持正则表达式模糊匹配）

- 支持指定集合的创建时间超过一定天数才迁移

- 支持指定集合超过一定天数没有数据写入才迁移记录

- 支持指定迁移的数据存储的数据域

- 支持设置分区中断（开启分区中断后，同一主表下的目标子表将按分区顺序进行迁移，当前面的分区不满足迁移条件时，后面的分区将被跳过）

#### 数据切换调度任务

数据切换调度任务支持周期调度将源站点上已经迁移到目标站点（数据源集群）上的集合，通过数据源关联的方式，重新创建一个关联到数据源集群的同名表，使得能够通过源集合访问到目标站点上的数据，从而实现数据切换；在这个过程中，源集合将被重命名，重命名的命名格式为 {clName}_data_switch_bak_{time}:

数据切换前后，访问集合数据时的数据访问路径如下：
![数据切换前示意图][before_data_switch]
![数据切换后示意图][after_data_switch]

- 由用户指定迁移的源站点和目标站点

- 支持设定要数据切换的集合空间/集合（支持正则表达式模糊匹配）

- 支持指定集合的创建时间超过一定天数才切换

- 支持指定集合超过一定天数没有数据写入才切换

- 支持设置分区中断（开启分区中断后，同一主表下的目标子表将按分区顺序进行数据切换，当前面的分区不满足数据切换条件时，后面的分区将被跳过）

#### 清理调度任务

清理调度任务是将已经完成数据切换，并且被数据切换任务重命名的源集合删除，重命名的命名格式为 {clName}_data_switch_bak_{time}:

![清理示意图][clean]

- 由用户指定清理的站点

- 支持设定要清理的集合空间/集合（支持正则表达式模糊匹配）

- 支持指定重命名集合的最大保留天数


[^_^]:
    本文使用到的所有链接及引用。
[system_architecture]:images/Distributed_Engine/Maintainance/Data_Archiving/system_architecture.png
[transfer]:images/Distributed_Engine/Maintainance/Data_Archiving/transfer.png
[before_data_switch]:images/Distributed_Engine/Maintainance/Data_Archiving/before_data_switch.png
[after_data_switch]:images/Distributed_Engine/Maintainance/Data_Archiving/after_data_switch.png
[clean]:images/Distributed_Engine/Maintainance/Data_Archiving/clean.png