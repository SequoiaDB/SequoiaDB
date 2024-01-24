[^_^]:   
    扩容和缩容
    作者：黄文华   
    时间：20190327
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：



扩容
----

当原有数据库集群容量无法满足业务需求时，用户可以通过[新增服务器][expand]并在[服务器内新增节点][add_node]，或直接在原有[服务器内新增节点][add_node]来扩容集群。扩容能够增加整个集群的存储空间，并提升数据库集群的处理效率。

缩容
----

当需要缩小原有数据集群时，需要集群缩容操作。集群缩容包括[服务器缩容][reduce]和[服务器内节点缩容][reduce_node]，其中服务器缩容包含服务器内节点缩容。服务器缩容将数据节点迁移到同集群其他服务器上进而卸载机器。服务器内节点缩容是指将数据迁移到同集群其他节点后直接删除节点。


[^_^]:
    本文使用到的所有链接及引用。
[expand]: manual/Distributed_Engine/Maintainance/Expand/expand.md
[add_node]: manual/Distributed_Engine/Maintainance/Expand/add_node.md
[reduce]: manual/Distributed_Engine/Maintainance/Expand/reduce.md
[reduce_node]: manual/Distributed_Engine/Maintainance/Expand/reduce_node.md
