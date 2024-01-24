[^_^]:   
    新增节点
    作者：黄文华   
    时间：20190327
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190716

SequoiaDB 巨杉数据库包括三类节点，集群可以通过在服务器内新增节点实现扩容：

* 新增协调节点  
* 新增编目节点  
* 新增数据节点 

新增协调节点
----

### 新增协调节点 ###

当集群规模随着新增服务器扩大时，[协调节点][coord_node]也需要随着规模的增加而进行增加，用户可以通过 SDB Shell 在现有协调节点组中添加新的协调节点。

以下示例为 sdbserver1 中已有协调节点或临时协调节点，在 sdbserver2 中添加新的协调节点，使用端口为 11810。

1. 连接 sdbserver1 的协调节点 11810

   ```lang-javascript
   > var db = new Sdb( 'sdbserver1', 11810 )  
   ```

2. 获取协调节点组  

   ```lang-javascript
   > var rg = db.getCoordRG()    
   ```

3. 在 sdbserver2 中新建协调节点 11810 

   ```lang-javascript
   > var node = rg.createNode( "sdbserver2", 11810, "/opt/sequoiadb/database/coord/11810" )
   ```

4. 启动新建的协调节点

   ```lang-javascript
   > node.start()
   ```

### 查看协调节点 ###

通过 SDB Shell 查看协调节点组的详细信息
 
```lang-javascript
> db.getCoordRG().getDetail()
```

新增编目节点
----

### 编目复制组中新增节点 ###

随着整个集群中的物理设备的扩展，用户可以通过增加更多的编目节点来提高编目服务的可靠性。如果新增[编目节点][catalog_node]涉及到新增服务器，用户需要先按照[操作系统配置][env_requirement]一节完成主机的主机名和参数配置。

1. 获取编目复制组

   ```lang-javascript
   > var cataRG = db.getCatalogRG() 
   ```

2. 在 sdbserver2 中新建编目节点，使用端口为 11800 

   ```lang-javascript
   > var node = cataRG.createNode( "sdbserver2", 11800, "/opt/sequoiadb/database/cata/11800" )
   ```

3. 启动新增的编目节点

   ```lang-javascript
   > node.start()  
   ```

### 查看编目节点 ###

通过 SDB Shell 查看编目复制组的详细信息

```lang-javascript
> db.getCataRG().getDetail()
```

新增数据节点
----

### 新增数据复制组 ###

一个集群中可以配置多个复制组，最大可配置 60,000 个复制组。通过增加复制组，可以充分利用物理设备进行水平扩展，实现 SequoiaDB 的线性水平扩展能力。如果新增[数据节点][data_node]涉及到新增服务器，用户需要先按照[操作系统配置][env_requirement]一节完成主机的主机名和参数配置。

1. 新建数据复制组，其中参数为复制组名

   ```lang-javascript
   > var dataRG = db.createRG( "dataGroup" )
   ```

   >**Note:**
   >
   > 数据复制组与编目复制组不同的是，该操作不会创建任何数据节点。

2. 在 sdbserver1 的数据复制组中新增一个数据节点，使用端口为 11820

   ```lang-javascript
   > dataRG.createNode( "sdbserver1", 11820, "/opt/sequoiadb/database/data/11820" )  
   ```

   >**Note:**
   >
   > 用户可以根据需要多次执行该命令来创建多个数据节点。

3. 启动数据节点

   ```lang-javascript
   > dataRG.start()
   ```

### 复制组中新增节点 ###

如果新增节点涉及到新增服务器等操作，用户需按照[操作系统配置][env_requirement]一节完成主机的主机名和参数配置。某些复制组可能在创建时设定的副本数较少，随着物理设备的增加，需要增加副本数以提高复制组数据可靠性。部署数据节点的详细信息请参考[集群模式][cluster_deployment]。

1. 获取数据复制组，参数 groupname 为数据复制组组名

   ```lang-javascript
   > var dataRG = db.getRG( <groupname> )  
   ```

2. 创建一个新的数据节点

   ```lang-javascript
   > var node = dataRG.createNode( <host>, <service>, <dbpath>, [config] )   
   ```

   > **Note:** 
   >
   > host、service、dbpath 和 config 的设置请参考方法[createNode()][add_group]。

3. 启动新增的数据节点

   ```lang-javascript
   sdb > node.start()  
   ```

### 查看数据节点 ###

通过 SDB Shell 可以查看某个的数据复制组的详细信息，其中参数 groupname 为数据复制组组名

```lang-javascript
> db.getRG( <groupname> ).getDetail()
```

数据负载均衡
----

数据负载均衡是指增加数据节点后将原集群中的数据切分到新的数据节点中，将数据打散得更均匀，充分利用集群优势达到性能的最大化。数据分区是指，新增数据复制组需要将集合中的数据导出然后重新创建集合再重新导入，以达到分区数据均衡的效果。复制组中新增节点使用数据切割命令（split 命令）即可做数据切分打散，下文重点介绍复制组中新增节点如何重新打散。  

### 分区方式  

数据分区有两种方式： 

- 范围分区（Range）  
- 散列分区（Hash）

> **Note：**
>
> 范围分区和散列分区详细介绍可参考[分区原理][sharding]。

### 数据切分  

默认情况下，一个集合会被创建在一个随机的数据复制组中。如果用户希望通过水平切分将该集合划分到其它复制组，则需要数据切分功能。

数据切分是一种将数据在线从一个复制组转移到另一个复制组的方式。在数据转移的过程中，查询所得的结果集数据可能会暂时不一致，但是 SequoiaDB 可以保证磁盘中数据的最终一致性。

范围分区和散列分区都支持两种切分方式：范围切分和百分比切分。

- 范围切分

  - 在范围切分时，范围分区使用精确条件（如字段 a）

      ```lang-javascript
      db.sample.employee.split( "src", "dst", { a: 10 }, { a: 20 } )
      ```

     > **Note：**
     >
     > - 集合 sample.employee 已经指定分区方式为"Range"。
     > - "src"和"dst"分别表示“数据原本所在复制组”和“数据将要切分到的目标复制组”
     > - 数据切分及分区上的数据范围皆遵循左闭右开原则。即：{a:10},{a:20} 代表迁移数据范围为[10, 20)

  - 在范围切分时，散列分区使用 Partition（分区数）条件

     ```lang-javascript
     db.sample.employee.split( "src", "dst", { Partition: 10 }, { Partition: 20 } )
     ```

     > **Note：** 
     >
     > 集合 sample.employee 已经指定分区方式为"Hash"。

- 百分比切分

  在百分比切分时，范围分区和散列分区执行的命令没有区别

   ```lang-javascript
   db.sample.employee.split( "src", "dst", 50 )
   ```

###检查切分信息

用户可以通过[集合快照][snapshot]检查集合切分信息。

```lang-javascript
coord.snapshot( SDB_SNAP_COLLECTIONS, { "Name": "CS.CL" } )
```


[^_^]:
    本文使用到的所有链接及引用。

[snapshot]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONS.md
[env_requirement]:manual/Deployment/env_requirement.md
[data_node]:manual/Distributed_Engine/Architecture/Node/data_node.md
[catalog_node]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[coord_node]:manual/Distributed_Engine/Architecture/Node/coord_node.md
[cluster_deployment]:manual/Deployment/cluster_deployment.md
[add_group]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md
[sharding]:manual/Distributed_Engine/Architecture/Sharding/architecture.md#分区方式
