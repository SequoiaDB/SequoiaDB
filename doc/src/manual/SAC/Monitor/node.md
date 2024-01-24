本文档主要介绍如何通过 SAC 进行节点监控。

节点列表
----
节点列表页面显示当前服务所有节点的状态、节点名、主机名、所属分区组、角色、集合数、记录数、Lob 数等基本信息，同时支持用户进行启停节点操作。
![节点列表][nodes_list_1]

> **Note:**
>
> - 需要排序时，用户可以通过点击列表表头根据字段进行排序。
> - 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。

  - 启动节点
    1. 点击上方 **启动节点** 按钮，打开启动节点的窗口
       ![启动节点][nodes_list_2]
    
    2. 通过下拉菜单选择要启动的节点后点击 **确定** 按钮

  - 停止节点
    1. 点击上方 **停止节点** 按钮，打开停止节点的窗口
       ![停止节点][nodes_list_3]
    
    2. 通过下拉菜单选择要停止的节点后点击 **确定** 按钮

> **Note:**
>
> 当前服务没有可启动节点时，将无法进行启动节点操作；当前节点没有可停止节点时，将无法进行停止节点操作。


分区组列表
----
分区组列表页面显示当前服务所有分区组的基本信息，同时支持用户进行启停分区组操作。

![分区组列表][groups_list_1]

  - 启动分区组
   1. 点击上方 **启动分区组** 按钮，打开启动分区组的窗口
     ![启动分区组][groups_list_2]

   2. 通过下拉菜单选择要启动的分区组后点击 **确定** 按钮

  - 停止分区组
   1. 点击上方 **停止分区组** 按钮，打开停止分区组的窗口
     ![停止分区组][groups_list_3]

   2. 通过下拉菜单选择要停止的分区组后点击 **确定** 按钮

   > **Note:**
   >
   > 选择停止 SYSCatalogGroup 后，SAC 服务将无法使用，用户需要手工启动 catalog 组。
     
   >![启动分区组][groups_list_4]
   
  > 在各机器上输入如下指令启动 catalog 组（默认情况下 catalog 节点的端口为 11820）：
   ```lang-bash
   $ sdbstart –p 11820
   ```

分区组快照
----
分区组快照支持查看当前服务的所有分区组的快照信息，用户可以自行选择需要显示在列表中的信息。同时，分区组快照支持筛选搜索功能和实时刷新功能。


需要实时刷新快照数据时，用户可以通过点击表格上方的 **启动刷新** 按钮

![分区组快照][groups_snapshot_1]

如果表格中的数据相比前一次有上升，那么该数据字体颜色为红色，下降则为绿色。


![节点快照实时刷新][groups_snapshot_2]

> **Note:**
>  
> - 需要排序时，用户可以通过点击列表表头根据字段进行排序。
> - 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。

节点快照
----
节点快照页面支持查看当前服务的编目节点和数据节点的快照信息，用户可以自行选择需要显示在列表中的信息。同时，节点快照支持筛选搜索功能和实时刷新功能。

需要实时刷新快照数据时，可以通过点击表格上方的 **启动刷新** 按钮

![节点快照][nodes_snapshot_1]

如果表格中的数据相比前一次上升，那么该数据字体颜色为红色，下降则为绿色

![节点快照实时刷新][nodes_snapshot_2]
 
> **Note:**
>  
> - 需要选择显示哪些字段，用户可以点击表格上方的 **选择显示列** 按钮。 
> - 需要排序时，用户可以通过点击表格表头来根据字段进行排序。  
> - 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。
> - 节点快照对应的字段说明，可通过[数据库快照][SDB_SNAP_DATABASE]查看。

分区组信息
----
分区组信息有分区组信息和分区组图表两个子页面。

- 分区组信息：支持查看所选分区组的详细信息、运行状态和属于该分区组的节点信息
- 分区组图表：支持查看所选分区组的增删改查近 30s 的实时速率

> **Note:**
> 
> 在节点列表中点击任意一个分区组即可查看对应的分区信息

###  分区组总览

![分区组信息][group_info_1]

该页面支持用户进行启停节点操作。

  - 启动节点

    1. 点击 **启动节点** 按钮

       ![启动节点][group_info_2]

    2. 选择要启动的节点后点击 **确定** 按钮

  - 停止节点

    1. 点击 **停止节点** 按钮

       ![停止节点][group_info_3]

    2. 选择要停止的节点后点击 **确定** 按钮

> **Note:**  
> - 当前服务没有可启动节点时，将无法使用 **启动节点** 操作。  
> - 当前节点没有可停止节点时，将无法使用 **停止节点** 操作。  
> - 分区组对应的字段说明可以通过[分区组列表][SDB_LIST_GROUPS]查看。

### 分区组图表
该页面显示分区组 30s 内增删改查的实时速率。
![分区组图表][group_info_4]

节点信息
----
节点信息有节点信息、节点会话、节点上下文和节点图表四个子页面。
  
  - 节点信息：可以查看所选节点的运行状态、详细信息、及节点增删改查操作的实时速率
  - 节点会话：查看所选节点的会话快照
  - 节点上下文：查看所选节点的上下文快照
  - 节点图表：查看所选节点的会话数量、上下文数量及事务数量的实时图表

> **Note:**
>
> 在节点列表中点击任意一个节点名即可查看对应的节点信息

### 节点总览

![节点信息][node_info_1]

### 节点会话

![节点会话][node_info_2]

点击【SessionID】可以查看所选会话的详细信息

![节点会话][node_info_2.1]

> **Note:**
>  
> - 点击 **选择显示列** 可以选择显示哪些字段。  
> - 列表中【Classify】列是为了更好的分类，并不是会话快照的字段。  
> - 会话快照默认显示非 Idle 状态和外部的会话（【Type】为"Agent"、"ShardAgent"、"ReplAgent"或"HTTPAgent"的会话），用户可以根据实际情况自定义过滤。  
> - 会话快照对应的字段说明可以在[会话快照][SDB_SNAP_SESSIONS]查看。

### 上下文

![节点上下文][node_info_3]

点击【CntextID】可以查看所选上下文的详细信息

![节点上下文][node_info_3.1]

> **Note:**  
> - 用户点击 **选择显示列** 可以选择显示哪些字段。  
> - 上下文快照对应的字段说明可以在[上下文快照][SDB_SNAP_CONTEXTS]查看。

### 节点图表  

该页面显示所选节点的会话、上下文和事务的实时数量图表。

![节点图表][node_info_4]

图表
----
服务图表显示当前服务近 30s 内增删改查的实时速率。通过该图表，用户可以直观地看出整个服务的增删改查性能情况。

![分区组图表][chart_1]

节点数据同步
----
节点数据同步页面支持检查该服务节点间的数据是否完全一致。

![数据同步][node_sync_1]

通过点击 **节点显示方式** 可以选择 **全部节点** 或者 **数据不同节点** 的查看方式，数据不同节点方式在列表中只显示数据不一致的节点，通过【CompleteLSN】字段检查数据的一致性，如下图所示：
![数据同步][node_sync_2]

> **Note:**   
> - 用户选择 **数据不同节点** 方式时，如果节点完全一致，列表中将不显示信息。 
> - 通过 **刷新间隔** 和 **启动刷新** 按钮，用户可以选择获取信息的时间间隔和是否重复获取。

[^_^]:
    本文使用的所有引用及链接
[SDB_SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[SDB_LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
[SDB_SNAP_SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[SDB_SNAP_CONTEXTS]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS.md

[nodes_list_1]:images/SAC/Monitor/nodes_list_1.png
[nodes_list_2]:images/SAC/Monitor/nodes_list_2.png
[nodes_list_3]:images/SAC/Monitor/nodes_list_3.png
[groups_list_1]:images/SAC/Monitor/groups_list_1.png
[groups_list_2]:images/SAC/Monitor/groups_list_2.png
[groups_list_3]:images/SAC/Monitor/groups_list_3.png
[groups_list_4]:images/SAC/Monitor/groups_list_4.png
[groups_snapshot_1]:images/SAC/Monitor/groups_snapshot_1.png
[groups_snapshot_2]:images/SAC/Monitor/groups_snapshot_2.png
[nodes_snapshot_1]:images/SAC/Monitor/nodes_snapshot_1.png
[nodes_snapshot_2]:images/SAC/Monitor/nodes_snapshot_2.png
[group_info_1]:images/SAC/Monitor/group_info_1.png
[group_info_2]:images/SAC/Monitor/group_info_2.png
[group_info_3]:images/SAC/Monitor/group_info_3.png
[group_info_4]:images/SAC/Monitor/group_info_4.png
[node_info_1]:images/SAC/Monitor/node_info_1.png
[node_info_2]:images/SAC/Monitor/node_info_2.png
[node_info_2.1]:images/SAC/Monitor/node_info_2.1.png
[node_info_3]:images/SAC/Monitor/node_info_3.png
[node_info_3.1]:images/SAC/Monitor/node_info_3.1.png
[node_info_4]:images/SAC/Monitor/node_info_4.png
[chart_1]:images/SAC/Monitor/chart_1.png
[node_sync_1]:images/SAC/Monitor/node_sync_1.png
[node_sync_2]:images/SAC/Monitor/node_sync_2.png

