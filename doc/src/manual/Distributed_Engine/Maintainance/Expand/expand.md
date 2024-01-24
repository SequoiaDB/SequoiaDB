[^_^]:   
    扩容
    作者：黄文华   
    时间：20190527
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190715


当原有的数据库集群无法满足业务需求时，用户可以通过新增服务器来扩容集群。新增服务器是指在现有的集群中添加新的主机，并把新的节点部署到这些新的主机上。

一般用户使用传统的 x86 服务器即可作为扩容的服务器，按照[操作系统要求][env_requirement]中的最低配置信息或者推荐配置来配置服务器。在分布式系统中三副本模式是最理想的服务器配置，三台服务器有利于 raft 算法选主，因此建议扩容按照三台服务器或者三的倍数台服务器来扩容集群。三台服务器中只保留一个主节点，另外两台服务器作为副本备份数据。综上所述，无论是经济上还是数据安全可靠性上三副本都是最合适的方案。 

扩容架构
----

集群从三台服务器扩容至六台服务器如图所示：

![架构图][add_host]

注意事项
----

在添加新的服务器的过程中，用户需要注意以下几点：

 - 需要在新的服务器上安装与其他主机相同的操作系统；
 - 新的服务器需要按照[操作系统要求][env_requirement]章节对操作系统进行配置，使所有主机可以两两通过主机名访问对方；
 - 在新的服务器上按照[数据库安装][database_deployment]章节对 SequoiaDB 进行安装时，配置管理服务端口需要与现有系统的端口保持一致（默认为 11790）；
 - 集群添加服务器后，需要在新的服务器上进行[服务器内新增节点][add_node]操作。


[^_^]:
    本文使用到的所有链接及引用。
[env_requirement]:manual/Deployment/env_requirement.md
[database_deployment]:manual/Deployment/engine_install.md
[add_node]:manual/Distributed_Engine/Maintainance/Expand/add_node.md
[add_host]:images/Distributed_Engine/Maintainance/Expand/add_host.PNG
