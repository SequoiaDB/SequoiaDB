[^_^]:
    核心特性


SequoiaDB 巨杉数据库作为一款分布式关系型数据库，支持如下特性：
+ [弹性水平扩展][scaleout]
+ [高可用与容灾][hadr]
+ [分布式事务][dist_tran]
+ [多模式接口][multi_model]
+ [混合负载][htap]
+ [多租户隔离][mlti_tenancy]

##弹性水平扩展##

作为一款分布式关系型数据库，支持无限弹性水平扩展是 SequoiaDB 巨杉数据库的基本特性，其底层的分布式存储引擎与上层的数据库实例均支持无限弹性水平扩展能力。

SequoiaDB 巨杉数据库的数据库实例均无状态并使用 TCP/IP 协议对外提供服务。为了提升整体处理能力，用户可以通过增加服务器数量或创建额外的数据库实例实现对应用的水平弹性扩张。

分布式存储引擎内部包含三种类型的节点：[协调节点][coord_node]、[编目节点][catalog_node]与[数据节点][data_node]。

其中协调节点主要作为数据请求的路由进程，对来自上层数据库实例的请求进行分发，并对数据节点返回的结果进行汇总。因此，每个协调节点均无状态，可以通过增加协调节点的数量提升数据路由层的处理能力。

编目节点默认使用三副本，由于只有当协调节点第一次访问某个表或集合时才需要读取编目节点，且建表与更改集群拓扑结构时才需要写入编目节点，因此在正常生产环境中编目节点的访问量极低，基本不可能成为瓶颈。

数据节点则采用水平分片的方式对数据进行横向切分，用户可以通过增加复制组或数据分片的方式，对数据引擎层的存储进行弹性水平扩展。

##高可用与容灾##

由于 PC 服务器内置物理磁盘不同于传统小型机加存储设备的架构，在 PC 服务器出现物理故障时无法保障存储在本地磁盘的数据不丢不坏，因此所有基于 PC 服务器内置盘架构的数据库，必须采用多副本机制以保障数据库的[高可用][ha]与[容灾][dr]。

###数据库实例###

由于数据库实例进程均为无状态节点，因此同样配置的数据库实例进程可以互为高可用冗余。不论创建 MySQL、PostgreSQL、JSON API 或 S3 实例，每个实例对外均可暴露一个或多个接入地址（IP 地址+端口）。应用程序连接到任意一个接入地址均可向数据库实例进行数据读写操作，且保证多个接入地址之间的数据一致性。用户可以为每个实例的多个接入地址前置一个类似 Ngnix 或 F5 的高可用负载均衡设备，即可轻易实现多个接入地址的高可用冗余。

###协调节点###

作为数据库存储引擎的路由节点，协调节点自身无状态，因此所有协调节点之间可以作为完全对等配置，对上层应用程序或计算引擎做到高可用。应用程序可以通过上层数据库实例访问数据，也可直接对数据库存储引擎进行 API 访问。当应用程序直接连接到协调节点进行 API 操作时，应用可以通过 SequoiaDB 巨杉数据库客户端连接池配置多个 IP 地址与端口实现高可用配置。如果应用通过上层数据库实例进行访问，所有数据库实例均支持多个接入地址的高可用的配置方式。

###编目节点###

编目节点作为数据字典，维护了 SequoiaDB 巨杉数据库存储引擎的拓扑结构、安全策略、表与集合定义以及分片规则等一系列信息。在 SequoiaDB 巨杉数据库的集群配置中，编目节点以一个独立复制组的方式存在，默认使用三副本强一致同步策略。在故障发生时，任何一个节点出现问题均可将服务实时漂移到其他的对等节点中。

###数据节点###

SequoiaDB 巨杉数据库中所保存的用户数据由数据节点进行存放与读取。在集群部署环境中，每个数据复制组均会默认使用三副本进行数据存放。在数据复制组中，任何一个数据节点进程出现故障，该复制组内的其他节点将会实时接管其服务。具体来说，如果发生故障的节点为该复制组内的主节点，则其余的从节点将会在检测到心跳中断后发起投票请求，使用 Raft 协议选举出新的主节点；而如果发生故障的为从节点，则协调节点检测到心跳中断后将会将该数据节点存在的会话转移至其余数据节点，尽可能对应用程序保持透明。

###异地容灾###

在传统的多节点[投票选举][vote]机制中，为了确保复制组内的节点不会发生脑裂问题，集群必须确保超半数节点存活且达成投票共识，才能将其中一个数据节点或编目节点当选为主节点以提供读写服务。但是在同城双中心或类似的环境下，用户很难保证在任何一个中心发生整体故障时，整个集群所有复制组依然会有超半数的节点存活。因此，SequoiaDB 巨杉数据库使用[集群分离与合并][split_merge]功能，能够在同城双中心的环境中进行秒级集群分裂，将原本处于两个数据中心内的单集群分裂为两个独立部署的集群，保证在存活数据中心内的数据服务能够以秒级启动并提供完整的数据库读写服务，同时保证交易数据的稳定可靠，做到秒级 RTO、RPO=0。

##分布式事务##

SequoiaDB 巨杉数据库支持强一致分布式事务功能。利用[二段提交][2pc]机制，SequoiaDB 巨杉数据库在分布式存储引擎实现了对结构化与半结构化数据的强一致分布式事务功能，不论用户创建哪种数据库实例，其底层均可提供完整的分布式事务及锁能力。

SequoiaDB 巨杉数据库完整支持三种[隔离级别][isolation]，同时支持读写锁等待以及读已提交版本机制。

##多模式接口##

SequoiaDB 巨杉数据库通过[数据库实例][instance]的形式提供多种关系型以及非关系型数据库兼容引擎，支持结构化、半结构化以及非结构化数据。在当前版本中，SequoiaDB 巨杉数据库支持包括 MySQL、MariaDB、PostgreSQL 以及 SparkSQL 在内的四种关系型数据库引擎，同时支持 JSON API 的半结构化数据引擎，以及 S3 对象存储的非结构化数据引擎。

使用多模式接口机制，用户可以使用 SequoiaDB 巨杉数据库服务于任何类型的应用程序，真正做到分布式数据库的平台化服务。

##混合负载##

一般来说，混合负载意味着数据库既可以运行 OLTP (Online Transactional Processing) 联机交易，也可以同时运行 OLAP (Online Analytical Processing) 统计分析业务。但是，用户想要在同一个数据库中针对同样的数据在同一时刻运行两种不同类型的业务，往往数据库服务器中的 CPU、内存、I/O 和网络等硬件资源会形成较多的资源争用，导致对外的联机交易服务性能与稳定性受到影响。

在 SequoiaDB 巨杉数据库中，用户可以针对复制组的多副本，在节点和会话等多个级别指定[读写分离][read_write]策略，同时可以通过创建数据共享但不同类型的数据库实例（例如 MySQL 实例与 SparkSQL 实例），分别服务于联机交易业务与统计分析业务，做到针对同样数据的联机交易与统计分析业务同时运行且互不干扰。

##多租户隔离##

对于分布式数据库来说，其存在的价值不仅仅在于解决单点数据量大的问题。更是在应用程序微服务化的今天，分布式数据库需要以一种平台化（PaaS）的形式对上层大量的应用与微服务同时提供数据访问能力。在这种情况下，如何做到不同微服务之间所对应的底层数据逻辑与物理隔离，是保障云环境中分布式数据库安全、可靠和性能稳定的前提。

在 SequoiaDB 巨杉数据库中，[数据域][domain]可以用于复杂集群环境中对资源进行逻辑与物理划分隔离。例如，在极为重要的联机交易型账务类应用中，其物理资源往往需要与审计后督类业务完全隔离，以保障在任何情况下审计类业务的复杂压力不会影响到核心账务系统的稳定运行。同样，不同的数据域之间的数据安全性配置、硬件资源环境等往往也不尽相同。

通过包括数据域、混合负载、多模式接口、水平弹性扩展在内的多种机制，SequoiaDB 巨杉数据库能够保障应用程序在云环境下的多租户隔离。

##小结##

总体来看，作为一款新一代金融级分布式关系型数据库，SequoiaDB 巨杉数据库除了高度兼容包括 MySQL 与 PostgreSQL 在内的多种传统数据库外，还在水平扩展、数据安全、分布式事务、多模式接口、混合负载以及多租户隔离等领域有着独特的优势。


[^_^]:
    本文使用到的所有链接及引用。
[coord_node]:manual/Distributed_Engine/Architecture/Node/coord_node.md
[catalog_node]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[ha]:manual/Distributed_Engine/Maintainance/HA_DR/high_avaliability.md
[dr]:manual/Distributed_Engine/Maintainance/HA_DR/disaster_recovery.md
[vote]:manual/Distributed_Engine/Architecture/Replication/election.md
[split_merge]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md
[2pc]:manual/Distributed_Engine/Architecture/Transactions/2pc.md
[isolation]:manual/Distributed_Engine/Architecture/Transactions/isolation.md
[instance]:manual/Database_Instance/Readme.md
[read_write]:manual/Distributed_Engine/Architecture/Replication/architecture.md#读写分离
[domain]:manual/Distributed_Engine/Architecture/domain.md
[scaleout]:manual/SequoiaDB_Intro/features.md#弹性水平扩展
[hadr]:manual/SequoiaDB_Intro/features.md#高可用与容灾
[dist_tran]:manual/SequoiaDB_Intro/features.md#分布式事务
[multi_model]:manual/SequoiaDB_Intro/features.md#多模式接口
[htap]:manual/SequoiaDB_Intro/features.md#混合负载
[mlti_tenancy]:manual/SequoiaDB_Intro/features.md#多租户隔离