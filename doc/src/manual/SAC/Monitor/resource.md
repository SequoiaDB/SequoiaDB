本文档主要介绍如何通过 SAC 进行资源监控。

会话
----
会话页面支持查看当前服务的所有会话快照。
![会话列表][sessions_list_1]

该列表显示当前服务的所有[会话快照][SDB_SNAP_SESSIONS]信息。
  - 点击 【SessionID】 可以查看所选会话的详细信息。
  - 需要选择显示哪些字段，用户可以点击列表上方的 **选择显示列** 按钮进行选择。
  - 需要排序时，用户可以通过点击列表表头来根据字段进行排序。
  - 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。

> **Note:**
>
> - 表格中 【Classify】 列是为了更好的分类而添加显示的字段，并不是会话快照自带的字段信息。
> - 会话快照默认显示非 Idle 状态和外部的会话(【Type】为"Agent"、"ShardAgent"、"ReplAgent"和"HTTPAgent"的会话)，可通过字段下方的筛选框选择显示所有会话。
> - 会话快照对应的字段说明可以通过[会话快照][SDB_SNAP_SESSIONS]查看。

上下文
----
上下文页面支持查看当前服务所有上下文快照，用户可以自行选择需要显示在列表中的信息。同时，该页面支持筛选搜索功能和实时刷新功能。
![上下文][contexts_list_1]

- 页面显示当前服务的所有上下文快照表格。
- 点击【CntextID】可以查看所选上下文的详细信息。
- 需要排序时，用户可以通过点击表格表头来根据字段进行排序。
- 需要搜索某个字段时，用户可以在所在字段上方的输入框输入关键字进行搜索。

> **Note:**
>
> 上下文快照对应的字段说明可以通过[上下文快照][SDB_SNAP_CONTEXTS]查看。

域
----
域页面支持查看当前服务下所有由用户创建的域的详细信息。用户可以进行创建域、删除域和编辑域操作。

### 页面信息

![域][domain_1]

该页面会列出所有由用户创建的域的基本信息，包括域的分区组列表、集合空间列表、集合列表和是否自动切分等信息。

需要了解某一个域的详细信息时，可点击域名【Name】打开该域的详细信息窗口。如下图：

![域信息][domain_2]

### 创建域

点击 **创建域** 按钮，在弹窗填写域名，选择是否自动切分及分区组后点击 **确定** 按钮完成创建域

![创建域][domain_3]

### 删除域

点击 **删除域** 按钮，选择要删除的域后点击 **确定** 按钮

![删除域][domain_4]

   > **Note:**
   >
   > 删除域前必须保证域中不存在任何数据，否则将删除失败。

### 编辑域

点击 **编辑域** 按钮，选择需要编辑的域，修改需要编辑的属性后，点击 **确定** 按钮

![编辑域][domain_5]

   > **Note:**
   > - 删除复制组前必须保证其不包含任何数据，否则将编辑失败。
   > - 是否更改自动切分对之前创建的集合和集合空间不会产生影响。

存储过程
----
存储过程页面支持查看当前服务的所有存储过程信息，用户可以进行创建和删除存储过程操作。

![存储过程][procedures_1]

该页面显示服务的所有存储过程信息，且存储过程函数在该页面将被格式化显示。

  - 创建存储过程
     
     点击表格上方的 **创建存储过程** 按钮，在输入框中输入要创建的完整函数后点击 **确定** 按钮
         ![创建存储过程][procedures_2]

 > **Note:**
 >
 >   - 函数必须包含函数名，不能使用匿名函数，如 `function(x,y) { return x+y; }`，否则将会创建失败。
 >   - 函数中所有标准输出和标准错误会被屏蔽。同时，不建议在函数定义或执行时加入输出语句，大量的输出可能会导致存储过程运行失败。
 >   - 函数返回值可以是除 db 以外任意类型数据，如 `function getCL() { return db.sample.employee; }`。

  - 删除存储过程
      
     在表格中点击要删除的存储过程【函数名】后点击 **删除** 按钮
         ![删除存储过程][procedures_3]


事务
----
事务页面支持查看当前服务所有的事务快照。该页面用列表的形式显示当前服务的所有事务快照信息，从列表中可以查看事务各种类型的锁的数量。

![事务][transactions_1]


- 需要查看单个事务的快照信息时，用户可以点击列表中的【事务ID】打开事务详细信息窗口
  ![事务详细][transactions_2]

- 需要查看单个事务的会话快照信息时，用户可以点击列表中的【会话ID】打开会话详细信息窗口
  ![会话详细][transactions_3]

> **Note:**
>
> 用户可以通过[事务快照][SDB_SNAP_TRANSACTIONS]和[会话快照][SDB_SNAP_SESSIONS]查看快照对应的字段说明。

图表
----
资源图表页面支持查看当前服务的会话、上下文、事务和存储过程近 30s 内的实时数量。
![资源图表][charts_1]



[^_^]:
    本文使用的所有引用和链接
[SDB_SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[SDB_SNAP_TRANSACTIONS]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS.md
[SDB_SNAP_SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[SDB_SNAP_CONTEXTS]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS.md

[sessions_list_1]:images/SAC/Monitor/sessions_list_1.png
[contexts_list_1]:images/SAC/Monitor/contexts_list_1.png
[domain_1]:images/SAC/Monitor/domain_1.png
[domain_2]:images/SAC/Monitor/domain_2.png
[domain_3]:images/SAC/Monitor/domain_3.png
[domain_4]:images/SAC/Monitor/domain_4.png
[domain_5]:images/SAC/Monitor/domain_5.png
[procedures_1]:images/SAC/Monitor/procedures_1.png
[procedures_2]:images/SAC/Monitor/procedures_2.png
[procedures_3]:images/SAC/Monitor/procedures_3.png
[transactions_1]:images/SAC/Monitor/transactions_1.png
[transactions_2]:images/SAC/Monitor/transactions_2.png
[transactions_3]:images/SAC/Monitor/transactions_3.png
[charts_1]:images/SAC/Monitor/charts_1.png