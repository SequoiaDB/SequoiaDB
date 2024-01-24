[^_^]:
    分布式事务
    作者：李伟超
    时间：20190222
    评审意见
    李伟超：初稿完成；时间：20190222
    检视：时间
    市场部：时间：20190329


在 SequoiaDB 巨杉数据库中，单条记录的操作是原子性的，因此对单条记录的操作不需要事务。然而，在许多应用场景中存在对更新多条记录时的原子性需求以及读取多条记录时的一致性需求。SequoiaDB 通过跨复制组的分布式事务对上述需求提供了支持。

事务提供了一种“要么全做，要么什么都不做”的机制。当事务被提交给了数据库，则数据库需要确保该事务中的所有操作都成功完成且其结果被永久保存在数据库中。如果事务中的任何一个操作没有成功完成，则事务中的所有操作都需要回滚到事务执行前的状态。

事务
----
事务是数据库执行过程中的一个逻辑单元，由一个有限的数据库读/写操作序列组成。事务有两个目的：

- 为数据库操作提供了一个从失败中恢复到正常状态的方法，同时提供了数据库即使在异常状态下仍能保持一致性的方法
- 当多个应用程序在并发访问数据库时，可以在这些应用程序之间提供一个隔离方法，以防止彼此的操作互相干扰

事务是恢复和并发控制的基本单位。数据库事务拥有以下四个特性，通常称为 ACID 特性：

- 原子性（Atomicity）：事务作为一个整体被执行，包含在其中的对数据库的操作要么全部被执行，要么都不执行
- 一致性（Consistency）：事务应确保数据库的状态从一个一致状态转变为另一个一致状态
- 隔离性（Isolation）：多个事务并发执行时，一个事务的执行不应影响其它事务的执行
- 持久性（Durability）：已被提交的事务对数据库的修改应当永久保存

SequoiaDB 事务中的操作只能是插入数据、修改数据以及删除数据，在事务过程中执行的其它操作不会纳入事务范畴，也就是说事务回滚时非事务操作不会被执行回滚。如果一个表或表空间中有数据涉及事务操作，则不允许删除该表或表空间。

在 SequoiaDB 中，默认情况下事务是开启的。关闭事务需要在目标节点上修改相关的[事务配置][configurations]。

SequoiaDB 的[事务日志][transaction_log]记录了事务对数据库的所有更改，是备份和恢复的重要组件。

原子性
----
原子性是数据库事务的最基本能力。事务必须确保在一个数据库会话中，从事务的开始到结束之间所有的操作为一个原子单元，不允许出现事务中的一部分成功，而另一部分失败的场景。

+ 当事务提交后，事务中的所有数据变更都被保存并且对外可见
+ 当事务回滚后，事务中的所有数据变更都被放弃并且不可见

隔离性
----
隔离性是避免在多个同时执行的事务操作会话之间出现相互干扰的机制。目前，SequoiaDB 支持三种隔离级别：

+ 读未提交（Read Uncommitted，RU）：RU 级别是最低隔离级别，意味着不同会话之间能够互相读到未提交的修改信息；
+ 读已提交（Read Committed，RC）：RC 级别为会话读取每条记录最新已被提交的状态；
+ 读稳定性（Read Stability，RS）：RS 级别为会话在事务中首次读取的记录，在该会话结束前不会被其他会话所修改。

详细可参考[隔离级别][isolation]。

分布式事务
----
作为分布式数据库，SequoiaDB 将数据分布式存储在一台或多台物理设备中。当事务中的操作发生在不同物理设备中时，SequoiaDB 使用[两阶段提交][2pc]协议实现分布式事务，支持跨表跨节点的事务原子操作。

事务操作
----
事务操作可参考 [SQL 应用开发的事务操作][sql_transactions]和 JSON 应用开发的事务操作。

SequoiaDB 事务支持的操作：

+ 插入（Insert）操作
+ 更新（Update）操作
+ 删除（Delete）操作
+ 查询（Find）操作

>**Note:**
>
> count 查询操作只支持 RU 和 RC 隔离级别。


[^_^]:
    本文使用到的所有链接

[transaction_log]: manual/Distributed_Engine/Architecture/Transactions/transaction_log.md
[isolation]: manual/Distributed_Engine/Architecture/Transactions/isolation.md
[2pc]: manual/Distributed_Engine/Architecture/Transactions/2pc.md
[configurations]: manual/Distributed_Engine/Architecture/Transactions/configurations.md
[sql_transactions]: manual/Manual/SQL_Grammar/Statement/transaction.md