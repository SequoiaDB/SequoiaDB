[^_^]:
    引擎调度单元
    作者：杨上德
    时间：20190313
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190726


引擎调度单元（简称 EDU，即 Engine Dispatchable Unit）是指 SequoiaDB 巨杉数据库引擎进程内主线程以外的单个线程，它是 SequoiaDB 中任务运行的载体。每个 EDU 可以用来执行用户的请求或者执行系统内部的维护任务。EDU 之间相互独立，不同的 EDU 单独负责不同的会话，每个 EDU 拥有一个进程内唯一的 64 位整数标识，即 EDU ID。

EDU 可以分为用户 EDU 和系统 EDU，分别代表执行用户任务的线程和执行系统任务的线程。EDU 的管理由 EDU 管理器完成，包括 EDU 的创建、监控、相互通讯及销毁等操作。

用户EDU
----

用户 EDU 为执行用户任务的线程，一般又叫作代理（Agent）线程。
在 SequoiaDB 中，主要存在下列代理线程类型：

|名称|类型|描述|
|---|----|---|
|Agent|代理|代理线程负责由 svcname 服务传入的请求，一般来说该请求由用户直接传入|
|ShardAgent|分区代理|分区代理线程负责由 shardname 服务传入的请求，一般来说该请求由协调节点传入数据节点|
|ReplAgent|复制代理|复制代理线程负责由 replname 服务传入的请求，一般来说该请求由数据主节点传向从节点，多作用于非协调节点|
|HTTPAgent|HTTP 代理|HTTP 代理线程负责由 httpname 服务传入的 REST 请求，一般来说该请求由用户直接传入|

系统EDU
----

系统 EDU 为系统内部维护数据结构及一致性的线程，一般来说对用户完全透明。
在 SequoiaDB 中，存在但不局限于下列系统 EDU：

|名称|类型|描述|
|---|----|---|
|TCPListener|服务监听|该线程负责监听 svcname 服务，并启动 Agent 代理线程|
|HTTPListener|HTTP 监听|该线程负责监听 httpname 服务，并启动 Agent 代理线程|
|Cluster|集群管理|集群管理线程用于维护集群的基本框架，启动 ReplReader 与 ShardReader 线程|
|ReplReader|复制监听|复制监听线程负责由 replname 服务传入的请求，并启动 ReplAgent 代理线程|
|ShardReader|分区监听|分区监听线程负责由 shardname 服务传入的请求，并启动 ShardAgent 代理线程|
|LogWriter|日志写|日志写线程用于将日志缓冲区中的数据写入日志文件|
|WindowsListener|Windows 事件监听|Windows 环境特有，用于监听 Windows 中 SequoiaDB 定义事件|
|Task|后台任务处理|后台任务处理线程，一般来说用于处理后台任务请求，例如：数据切分|
|CatalogManager|编目控制|编目控制线程用于处理编目节点内部元数据相关的请求|
|CatalogNetwork|编目网络监听|编目网络监听线程用于监听编目服务 catalogname 下的请求|
|CoordNetwork|协调网络监听|协调网络监听线程用于监听分区的请求|

后台任务EDU
----
对于一些需要在后台持续运行，或是执行时间较长的操作，SequoiaDB 采用[后台任务（Task）][background]的方式进行处理。

EDU 状态
----
为了方便监控，每个 EDU 都存在一个当前状态，这些状态包括：

+ Creating：创建状态，创建 EDU，但还未被激活时，处于该状态，通常在完成资源分配和一系列初始化动作后，就会被激活，进入到 Running 状态
+ Running：运行状态，EDU 被激活且进入任务的正式处理流程中，处于该状态
+ Waiting：等待状态，运行阶段完成后，进入该状态
+ Idle：空闲状态，用户连接断开后，处于该状态，该 EDU 可能已被放到会话池中
+ Destroying：销毁状态，EDU 正在被销毁

为了提高 EDU 的分配速度，使用 EDU 池缓存一定数据的空闲 EDU。当有新的用户连接建立时，可以直接从池中取出一个 EDU 进行使用。如果池为空，则单独创建一个 EDU。

EDU监控
----

用户可以使用[会话快照][snapshot_session]监控系统与用户 EDU。


[^_^]:
    本文使用到的所有链接及引用
[background]:manual/Distributed_Engine/Architecture/Thread_Model/background.md
[snapshot_session]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md