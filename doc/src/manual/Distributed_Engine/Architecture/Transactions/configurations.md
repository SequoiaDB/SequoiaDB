事务是由一系列操作组成的逻辑工作单元。在同一个会话（或连接）中，同一时刻只允许存在一个事务，也就是说当用户在一次会话中创建了一个事务，在这个事务结束前用户不能再创建新的事务。

事务作为一个完整的工作单元执行，事务中的操作要么全部执行成功要么全部执行失败。SequoiaDB 巨杉数据库中事务的操作只能是插入数据、修改数据以及删除数据，在事务过程中执行的其它操作不会纳入事务范畴，也就是说事务回滚时非事务操作不会被执行回滚。如果一个表或表空间中有数据涉及事务操作，则该表或表空间不允许被删除。

##事务启停##

数据库配置中，关于事务启停的配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| :----- | :--- | :--- | :----- |
| [transactionon][cluster_config]  | 表示 SequoiaDB 是否开启事务功能。 | true/false | true |

默认情况下，SequoiaDB 所有节点的事务功能都是开启的。若用户不需要使用事务功能，可参考示例 3 全局关闭事务的方法，关闭事务功能。

>**Note：**
>
>下述所有配置项只有在事务功能开启（即 transactionon 为 true）的情况下才有意义。

##隔离级别##

数据库配置中，关于事务隔离级别的配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| :----- | :--- | :--- | :----- |
| [transisolation][cluster_config] | 表示在开启事务的情况下，使用的事务隔离级别。 | 0 表示 RU，1 表示 RC，2 表示 RS | 0 |

用户可以通过 [Sdb.setSessionAttr()][set_session_attr] 的 TransIsolation 修改单个会话的隔离级别设置

> **Note:**
>
> 事务的隔离级别可参考[隔离级别][isolation]

##事务自动回滚##

对于写事务操作，若在操作过程发生错误，数据库配置中的 transautorollback 配置项可以决定当前会话所有未提交的写操作是否自动回滚。transautorollback 的描述如下：

| 配置项 | 描述 | 取值 | 默认值 |
| :----- | :--- | :--- | :----- |
| [transautorollback][cluster_config] | 表示写事务操作失败时，是否自动回滚。 | true/false | true |

用户可以通过 [Sdb.setSessionAttr()][set_session_attr] 的 TransAutoRollback 修改单个会话的事务自动回滚设置。

>**Note：**
>
> - 该配置项只有在事务功能开启（即 transactionon 为 true ）的情况下才生效。
> - 默认情况下，transautorollback 配置项的值为 true。所以，当写事务操作过程出现失败时，当前事务所有未提交的写操作都将被自动回滚。

##事务自动提交##

数据库配置中，关于事务自动提交的配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| :----- | :--- | :--- | :----- |
| [transautocommit][cluster_config] | 表示是否开启事务自动提交功能。 | true/false | false |

用户可以通过 [Sdb.setSessionAttr()][set_session_attr] 的 TransAutoCommit 修改单个会话的事务自动提交设置。

>**Note：**
>
>- 该配置项只有在事务功能开启（即 transactionon 为 true ）的情况下才生效。
>- 事务自动提交功能默认情况下是关闭的。当 transautocommit 设置为 true 时，事务自动提交功能将开启。此时，使用事务存在以下两点不同：
  - 用户不需要显式调用 "transBegin" 和 "transCommit" 或者 "transRollback" 方法来控制事务的开启、提交或者回滚。
  - 事务提交或者回滚的范围仅仅局限于单个操作。当单个操作成功时，该操作将被自动提交；当单个操作失败时，该操作将被自动回滚。

##其它配置##

数据库配置中，关于事务的其它主要配置项如下：

| 配置项 | 描述 | 取值 | 默认值 |
| :----- | :--- | :--- | :----- |
| transactiontimeout | 事务锁等待超时时间（单位：秒） | [0, 3600] | 60 |
| translockwait | 事务在 RC 隔离级别下是否需要等锁。 | true/false | false |
| transuserbs | 事务操作是否使用回滚段。 | true/false | true |
| transrccount | 事务是否使用读已提交来处理 count() 查询。 | true/false | true |
| transmaxlocknum | 事务在一个数据节点上最多可以持有的记录锁个数。 | [ -1, 2^31 - 1 ] <br> -1 表示事务对记录锁的个数没有限制。0 表示事务不使用记录锁，直接使用集合锁。 | 10000 |
| transallowlockescalation | 事务持有的记录锁个数超过参数 transmaxlocknum 的值后，是否允许锁升级。 | true/false | true |
| transmaxlogspaceratio | 事务在一个数据节点上可以使用的最大日志空间比例(%)。 | [ 1, 50 ]，取值为50，表示一半的日志空间都可以被事务使用，另外一半用于事务回滚。 | 50 |

>**Note：**
>
>+ 上表中的事务配置项只有在事务功能开启（即 transactionon 为 true ）的情况下才生效。
>+ translockwait 只在 RC 隔离级别下生效
>  + 在 RC 隔离级别下发生加锁冲突时，如果本事务的 translockwait 为 true 或者并发事务的 transuserbs 为 false （并发事务不使用回滚段） 时，当前事务需要等锁，待并发事务提交或者回滚后，才能加锁读取数据记录，此时读取的是并发事务提交或者回滚后的数据记录。
>  + 在 RC 隔离级别下发生锁冲突时，如果本事务的 translockwait 为 false 且并发事务的 transuserbs 为 true （并发事务使用回滚段）时，当前事务不需要等锁即可以读取数据记录（不能修改），此时读取的数据记录是并发事务修改前的数据记录（即保存在并发事务回滚段中的已提交版本）。
>  + 在 RU 隔离级别下，发生加锁冲突时，当前事务不等锁，但读取的是未提交事务的脏数据。
>  + 在 RS 隔离级别下，发生加锁冲突时，当前事务需要等锁，待并发事务提交或者回滚后，才能加锁读取数据记录，此时读取的时并发事务提交或者回滚后的数据记录。

上表中的事务配置项可以通过 [Sdb.setSessionAttr()][set_session_attr] 对应的配置项对单个会话的事务配置进行修改。

##会话级别设置##

用户可以通过 [Sdb.setSessionAttr()][set_session_attr] 来设置单个会话的事务相关的设置：

| 属性名 | 描述 | 格式 |
| :----- | :--- | :--- |
| TransIsolation | 会话事务的隔离级别，0 为 RU 级别，1 为 RC 级别，2 为 RS 级别。 | TransIsolation : 1 |
| TransTimeout | 会话事务锁等待超时时间（单位：秒）。 | TransTimeout : 10 |
| TransLockWait | 会话事务在 RC 隔离级别下是否需要等锁。 | TransLockWait : true |
| TransUseRBS | 会话事务是否使用回滚段。 | TransUseRBS : true |
| TransAutoCommit | 会话事务是否支持自动事务提交。 | TransAutoCommit : true |
| TransAutoRollback | 会话事务在操作失败时是否自动回滚。 | TransAutoRollback : true |
| TransRCCount | 会话事务是否使用读已提交来处理 count() 查询。 | TransRCCount : true |
| TransMaxLockNum | 事务在一个数据节点上最多可以持有的记录锁个数。 | TransMaxLockNum : 10000 |
| TransAllowLockEscalation | 事务持有的记录锁个数超过参数 transmaxlocknum 的值后，是否允许锁升级。 | TransAllowLockEscalation : true |
| TransMaxLogSpaceRatio | 事务在一个数据节点上可以使用的最大日志空间比例(%)。 | TransMaxLogSpaceRatio : 50 |

>**Note：**
>
> - 上表中的事务配置项只有在事务功能开启（即 transactionon 为 true ）的情况下才生效
> - 修改 transactionon 需要重启节点

用户可以使用 [Sdb.getSessionAttr()][get_session_attr] 来获取单个会话的事务相关配置。

##调整设置##

当用户希望调整事务的设置时（如：是否开启事务、调整事务配置项等），有如下三种修改方式供用户选择：

- 通过修改节点配置文件，将数据库配置描述的事务配置项，配置到集群所有（或者部分）节点的配置文件中。若修改的配置项要求重启节点才能生效，用户需重启相应的节点。
- 通过使用 [Sdb.updateConf()][update_conf] 命令在 SDB Shell 中修改集群的事务配置项。若修改的配置项要求重启节点才能生效，用户需重启相应的节点。
- 通过使用 [Sdb.setSessionAttr()][set_session_attr] 命令在 SDB Shell 的会话中修改当前会话的事务配置项。该设置只在当前会话生效，并不影响其它会话的设置情况。

**示例**

- 假设集群的安装目录为 `/opt/sequoiadb`，协调节点地址为 `ubuntu-dev1:11810`，通过如下操作，获取 db 以及 cl 对象：
 
   ```lang-bash
   > db = new Sdb( "ubuntu-dev1", 11810 )
   > cl = db.createCS("sample").createCL("employee")
   ```

- 使用事务回滚插入操作，事务回滚后，插入的记录将被回滚，集合中无记录

   ```lang-bash
   > cl.count()
   Return 0 row(s). 
   > db.transBegin()
   > cl.insert( { date: 99, id: 8, a: 0 } )
   > db.transRollback()
   > cl.count()
   Return 0 row(s). 
   ```

- 使用事务提交插入操作，提交事务后，插入的记录将被持久化到数据库

   ```lang-bash
   > cl.count()
   Return 0 row(s). 
   > db.transBegin()
   > cl.insert( { date: 99, id: 8, a: 0 } )
   > db.transCommit()
   > cl.count()
   Return 1 row(s). 
   ```

- 全局关闭事务

   1. 通过 SDB Shell 设置集群所有节点都关闭事务

     ```lang-bash
     > db.updateConf( { transactionon: false }, { Global: true } )
     ```

   2. 在集群每台服务器上都重启 SequoiaDB 的所有节点

     ```lang-bash
     [sdbadmin@ubuntu-dev1 ~]$ /opt/sequoiadb/bin/sdbstop -t all
     [sdbadmin@ubuntu-dev1 ~]$ /opt/sequoiadb/bin/sdbstart -t all
     ```
  >**Note:**
  >
  > 必须在每台服务器上都重启 SequoiaDB 的所有节点，才能保证事务功能在所有节点上都是关闭的。

[^_^]:
    本文使用到的所有链接
[cluster_config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[isolation]:manual/Distributed_Engine/Architecture/Transactions/isolation.md
[update_conf]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
[set_session_attr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[get_session_attr]:manual/Manual/Sequoiadb_Command/Sdb/getSessionAttr.md
