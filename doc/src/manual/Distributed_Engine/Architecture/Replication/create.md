[^_^]:
    部署复制组
    作者：余婷
    时间：20190327
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190412

用户可以通过 SDB Shell 或者其他驱动创建管理复制组，添加删除复制组节点，以及查看复制组状态。

创建复制组
----

推荐创建三个副本的复制组，三个副本提供足够的冗余，足以承受系统故障。副本集推荐奇数个成员，以确保[选举][election]的顺利进行。

复制组的节点可以以多种角色运行，其中[数据节点][data]包含用户数据，[编目节点][catalog]包含系统数据。编目复制组一般在[集群部署][cluster_deploy]时创建完成，本章以创建数据复制组为例。

> **Note:**
>
> 创建节点的主机上需要先完成[集群部署][cluster_deploy]。

1. 连接协调节点

   ```lang-javascript
   > var db = new Sdb( 'sdbserver1', 11810 )
   ```
2. 创建数据复制组：group1

   ```lang-javascript
   > var rg = db.createRG( "group1" )
   ```
3. 创建三个复制组节点

   ```lang-javascript
   > rg.createNode( "sdbserver1", 11820, "/opt/sequoiadb/database/data/11820" )
   > rg.createNode( "sdbserver2", 11820, "/opt/sequoiadb/database/data/11820" )
   > rg.createNode( "sdbserver3", 11820, "/opt/sequoiadb/database/data/11820" )
   ```

4. 启动复制组

   ```lang-javascript
   > rg.start()
   ```

创建与删除节点
----

在复制组运行过程中，用户可以创建或者删除节点。创建节点时，可以指定 [weight][weight] 参数设置节点选举权重，权重高的节点将会优先成为复制组的主节点。

- **创建节点**

   1. 连接数据复制组：group1

      ```lang-javascript
      > var rg = db.getRG( "group1" )
      ```

   2. 创建新节点，并设置选举权重

      ```lang-javascript
      > var node = rg.createNode( "sdbserver4", 11820, "/opt/sequoiadb/database/data/11820", { weight: 20 } )
      ```

   3. 启动该节点

      ```lang-javascript
      > node.start()
      ```

- **删除节点**

   1. 连接数据复制组：group1
   
      ```lang-javascript
      > var rg = db.getRG( "group1" )
      ```
   
   2. 删除节点
   
      ```lang-javascript
      > rg.removeNode( "sdbserver4", 11820 )
      ```

分离与添加节点
----

当节点出现故障时，如出现磁盘损坏等情况，为避免影响复制组的正常运行，可以先将故障节点分离出复制组。待节点恢复后，可将该节点重新添加回复制组。

- **分离节点**

   1. 连接数据复制组：group1
   
      ```lang-javascript
      > var rg = db.getRG( "group1" )
      ```
   
   2. 从复制组中分离 `sdbserver3:11820` 节点
   
      ```lang-javascript
      > rg.detachNode( "sdbserver3", 11820 )
      ```

- **重新添加节点**

   1. 连接数据复制组：group1
   
      ```lang-javascript
      > var rg = db.getRG( "group1" )
      ```
   
   2. 把 sdbserver3:11820 节点加入到复制组
   
      ```lang-javascript
      > rg.attachNode( "sdbserver3", 11820 )
      ```

查看复制组状态
----

1. 连接数据复制组：group1

   ```lang-javascript
   > var rg = db.getRG( "group1" )
   ```

2. 查看该复制组主节点

   ```lang-javascript
   > rg.getMaster()
   ```
   
   输出结果如下：

   ```lang-text
   sdbserver1:11820
   ```

3. 查看该复制组信息

   ```lang-javascript
   > rg.getDetail()
   ```

   > **Note:** 
   >
   > getDetail() 返回字段的说明可参考[复制组列表][SDB_LIST_GROUPS]。



[^_^]:
    本文使用到的所有链接及引用。
[data]:manual/Distributed_Engine/Architecture/Node/data_node.md
[catalog]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[cluster_deploy]:manual/Deployment/cluster_deployment.md
[election]:manual/Distributed_Engine/Architecture/Replication/election.md
[weight]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[SDB_LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
