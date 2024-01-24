[^_^]:
    复制组选举


复制组通过选举机制来确定某一成员成为主节点，保障复制组内任何时候只存在一个主节点。当主节点故障后，会在其他备节点之间自动选举出主节点。

SequoiaDB 巨杉数据库使用改进过的 Raft 选举协议。

##节点心跳##

在复制组内，所有的节点定期向组内其它成员发送携带自身状态信息的消息，对端节点收到该消息后返回应答消息。节点两两之间通过该消息感知对方的状态，发送的消息称为心跳，其间隔为 2 秒钟。

![心跳示意图][heartbeat]

##节点选举##

当主节点发生故障，如主节点所在的机器宕机，其余节点一段时间内收不到主节点心跳后就会发起复制组选举。所有的备节点会进行投票，日志最接近原主节点的备节点会成为新的主节点。在选出主节点之前，复制组无法提供写操作服务。

![选举示意图][vote]

选举成功的前提条件是，组内必须有超过半数以上的节点参与投票。当出现复制组脑裂的情况下，可能同时存在两个主节点。

例如，复制组内有四个节点，节点 A 是主节点，因为网络中断导致脑裂现象，节点 A 只能与节点 B 通信，节点 C 只能与节点 D 通信。

![脑裂示意图][head_split]

当组内成员不足半数，主节点会自动降级为备节点，同时断开主节点的所有用户连接。

##选举权重##

用户可以通过配置 [weight][weight] 参数调整节点在复制组选举中的权重。


[^_^]:
    本文使用到的所有链接及引用。
[heartbeat]:images/Distributed_Engine/Architecture/Replication/heartbeat.png
[vote]:images/Distributed_Engine/Architecture/Replication/vote.png
[head_split]:images/Distributed_Engine/Architecture/Replication/head_split.png
[weight]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
