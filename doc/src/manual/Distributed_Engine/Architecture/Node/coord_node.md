
协调节点为一种逻辑节点，其中并不保存任何用户数据信息。

协调节点作为数据请求部分的协调者，本身并不参与数据的匹配与读写操作，而仅仅是将请求分发到所需要处理的数据节点。

一般来说，协调节点的处理流程如下：

1. 得到请求
2. 解析请求
3. 本地缓存查询该请求对应集合的信息
4. 如果信息不存在则从编目节点获取
5. 将请求转发至相应的数据节点
6. 从数据节点得到结果
7. 把结果汇总或直接传递给客户端

协调节点与其它节点之间主要使用分区服务端口（--shardname 参数）进行通讯。

SequoiaDB 巨杉数据库中有两类协调节点：

- 临时协调节点：通过资源管理节点 sdbcm 建立的协调节点。临时协调节点并不会注册到编目节点中，即该临时的协调节点不能被集群管理。临时协调节点仅用于初始创建 SequoiaDB 集群使用。
- 协调节点：通过正常的流程创建的协调节点组中的协调节点。该类协调节点会注册到编目节点中，并且可以被集群管理。

## 管理协调节点 ##

### 创建临时协调节点 ###

创建 SequoiaDB 集群时，用户可以在 SDB Shell 中通过 sdbcm 创建临时协调节点。

1. 连接到本地的集群管理服务进程 sdbcm

   ```lang-javascript
   > var oma = new Oma( "localhost", 11790 )
   ```

2. 创建临时协调节点

   ```lang-javascript
   > oma.createCoord( 18800, "/opt/sequoiadb/database/coord/18800" )
   ```

3. 启动临时协调节点

   ```lang-javascript
   > oma.startNode( 18800 )
   ```
>   **Note:**
>
>   创建临时协调节点可参考 [Oma.createCoord\(\)][createCoord]

### 创建协调节点组 ###

用户在 Sdb Shell 中可以通过临时协调节点可以创建协调节点组。

1. 连接临时协调节点

   ```lang-javascript
   > var db = new Sdb( "localhost", 18800 )
   ```

2. 创建协调节点组

   ```lang-javascript
   > db.createCoordRG()
   ```

>   **Note:**
>
> 创建协调节点组应先在集群中创建有效的编目节点，可参考[集群模式][cluster_deployment]


### 新增协调节点 ###

当集群规模扩大时，协调节点也需要随着规模的增加而进行增加。建议在每台物理机器上都配置一个协调节点。

在 Sdb Shell 中可以通过现有的协调节点组添加新的协调节点（假设有 sdbserver1 和 sdbserver2 两台处于同一个集群的服务器，sdbserver1 中已有协调节点（端口为 11810），现在向 sdbserver2 中添加新的协调节点）：

1. 连接 sdbserver1 的协调节点

   ```lang-javascript
   > var db = new Sdb( 'sdbserver1', 11810 )
   ```

2. 获取协调节点组

   ```lang-javascript
   > var rg = db.getCoordRG()
   ```

   >   **Note:**
   >
   >   获取协调节点组，可参考 [Sdb.getRG\(\)][getRG] 

3. 在 sdbserver2 中新建协调节点

   ```lang-javascript
   > var node = rg.createNode( "sdbserver2", 11810, "/opt/sequoiadb/database/coord/11810" )
   ```

4. 启动 sdbserver2 的协调节点

   ```lang-javascript
   > node.start()
   ```

### 查看协调节点 ###

在 SDB Shell 中查看协调节点的列表

```lang-javascript
> db.getCoordRG().getDetail()
```

## 故障恢复 ##

由于协调节点不存在用户数据，因此发生故障后可以直接重新启动，不参与任何额外的故障恢复步骤。


[^_^]:
     本文所有引用和链接
[createCoord]:manual/Manual/Sequoiadb_Command/Oma/createCoord.md
[cluster_deployment]:manual/Deployment/cluster_deployment.md
[getRG]:manual/Manual/Sequoiadb_Command/Sdb/getRG.md
