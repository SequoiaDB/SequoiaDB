[^_^]:
    线程模型
    作者：杨上德
    时间：20190313
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190604

一个数据库管理系统通常服务于多个用户，数据库如何调度和处理来自多个用户的请求以及系统内部的各种后台任务，会极大地影响系统的运行性能及用户的使用体验。

SequoiaDB 巨杉数据库使用的是多线程模型，包括系统线程和用户线程。在分布式模式下，用户每建立一个新的连接，并通过该连接向集群发送请求，集群中的不同节点上，会启动多个不同类型的线程为用户服务。

通过阅读本章，可以了解系统中线程的管理机制及相关组件的交互流程。本章主要内容如下：

+ [线程模型简介][intro]
+ [引擎调度单元][edu]
+ [会话][session]
+ [后台任务][background]


[^_^]:
    本文使用到的所有链接及引用。
[intro]:manual/Distributed_Engine/Architecture/Thread_Model/intro.md
[edu]:manual/Distributed_Engine/Architecture/Thread_Model/edu.md
[session]:manual/Distributed_Engine/Architecture/Thread_Model/session.md
[background]:manual/Distributed_Engine/Architecture/Thread_Model/background.md
