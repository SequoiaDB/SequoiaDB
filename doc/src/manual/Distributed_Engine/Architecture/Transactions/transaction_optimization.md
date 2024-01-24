[^_^]:
    大事务优化

实际业务场景中，大事务操作不可避免。在进行大事务操作时，集合中会有大量[记录锁][isolation]被请求，大大增加数据库因管理锁而导致的资源消耗。为此，SequoiaDB 通过锁升级和日志限额两方面对大事务的处理机制进行优化，以解决大事务资源消耗过大的问题。

- 锁升级（lock escalation）是数据库的一种作用机制，用于减少大量记录锁引起的内存开销。当一个集合中有大量记录锁被请求时，SequoiaDB 将用一个表锁代替该集合上的若干记录锁，减少数据库的资源消耗，实现资源的释放。用户可通过配置事务可持有的记录锁数量（对应参数 [transmaxlocknum][parameter_instructions]），决定加锁粒度。参数 transmaxlocknum 的取值越大，允许消耗的资源越多，发生锁升级的频率越低；取值越小，允许消耗的资源越少，发生锁升级的频率越高。触发锁升级后，事务间并发性将随着表锁数量的增加而降低。

- 日志限额通过限制大事务可使用的日志量（对应参数 [transmaxlogspaceratio][parameter_instructions]），以控制大事务的资源消耗。参数 transmaxlogspaceratio 的取值越小，集群中所允许的事务规格就越小，将阻止大事务的发生。

在大事务的优化中，参数 transmaxlocknum 和 transmaxlogspaceratio 的默认值即为 SequoiaDB 的推荐配置。执行大事务的过程中，用户可通过[事务快照][SDB_SNAP_TRANSACTIONS]查看事务是否已触发锁升级（对应参数 IsLockEscalated）、已使用的日志空间（对应参数 UsedLogSpace）、为回滚操作保留的日志空间（对应参数 ReservedLogSpace）等事务进展情况。当事务处于回滚状态时，事务对应的 ReservedLogSpace 值将会逐渐减小为 0，因此用户可通过 ReservedLogSpace 的值知晓当前事务回滚的进度。




[^_^]:
    本文使用到的所有链接
[isolation]:manual/Distributed_Engine/Architecture/Transactions/isolation.md
[parameter_instructions]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[SDB_SNAP_TRANSACTIONS]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS.md