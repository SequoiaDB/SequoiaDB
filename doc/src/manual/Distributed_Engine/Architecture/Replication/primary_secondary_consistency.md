[^_^]:
    主备一致性
    作者：余婷
    时间：20190527
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190723


在分布式系统中，一致性是指数据在多个副本之间数据保持一致的特性。SequoiaDB 巨杉数据库支持不同级别的主备一致性策略，以适配不同的应用场景。用户可根据业务对数据安全性和服务可用性的要求，选择不同的一致性策略。

强一致性
----

### 写所有节点 ###

当发生写操作时，数据库会确保所有复制组节点都同步完成才返回。写操作处理成功后，后续读到的数据一定是当前复制组内最新的。优势是能够有效地保证数据的完整性和安全性，劣势则是会降低复制组的写入性能，并且当集群内有一个节点故障或者异常时，无法写入数据，降低高可用性。

在联机交易型业务中，为了保证数据安全性，同时可以牺牲写入性能时，推荐使用强一致性策略。

![强一致性示意图][consistency0]

集合的 ReplSize 参数描述了在写操作返回成功之前，写操作执行成功了的节点个数。可以将 ReplSize 设为 0，或者设为复制组节点个数。以下以复制组三副本为例：

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> db.sample.createCL( 'employee1', { ReplSize: 0 } )
> db.sample.createCL( 'employee2', { ReplSize: 3 } )
```

### 写活跃节点 ###

为了防止某个节点突然故障，导致数据库完全不可用，可以将 ReplSize 设为 -1，指的是写所有的活跃节点。例如，复制组有三副本，某个备节点因为磁盘不足而异常停止，ReplSize 为 -1 时，写操作会写入另外 2 个非故障节点后才返回。

![写活跃节点示意图][consistency_alive]

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> db.sample.createCL( 'employee', { ReplSize: -1 } )
```

最终一致性
----

为了提升数据库的高可用性，以及实现数据的读写分离，SequoiaDB 默认采用“最终一致性”策略。在读写分离时，读取的数据在某一段时间内可能不是最新的，但副本间的数据最终是一致的。

### 写主节点 ###

在主节点执行写操作成功后，写操作即可返回。对数据查询一致性要求不高的业务，如历史数据查询平台，夜间批量导入数据以及白天提供查询业务，推荐使用写主节点的最终一致性策略。

![写主节点示意图][consistency1]

创建集合时，如果不设置 ReplSize，则默认值为 1

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> db.sample.createCL( 'employee' )
```

### 写多数派 ###

为了尽量保证数据的安全性，又兼顾高可用性，用户可以将 ReplSize 设为多数派。对数据一致性要求较高的业务，如影像内容管理平台和联机交易服务平台等，推荐使用写多数派的最终一致性策略。

![写多数派示意图][consistency2]

以三副本复制组为例，多数派 >= ( 组内节点总数 / 2 ) + 1 ，即 ReplSize 设为 2

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> db.sample.createCL( 'employee', { ReplSize: 2 } )
```


修改一致性策略
----
用户可以通过 [db.setAttributes()][session_attr] 修改 ReplSize 属性。

```lang-javascript
> var db = new Sdb ( 'sdbserver1', 11810 )
> db.sample.createCL( 'employee', { ReplSize: 1 } )
> db.sample.employee.setAttributes( { ReplSize: -1 } )
> db.snapshot( SDB_SNAP_CATALOG, { Name: "sample.employee" } )
{
  "_id": {
    "$oid": "5247a2bc60080822db1cfba2"
  },
  "Name": "sample.employee",
  "UniqueID": 261993005057,
  "Version": 1,
  "ReplSize": -1,
  "Attribute": 0,
  "AttributeDesc": "",
  "CataInfo": [
    {
      "GroupID": 1000,
      "GroupName": "group1"
    }
  ]
}
```

[^_^]:
    本文使用到的所有链接及引用。
[consistency0]:images/Distributed_Engine/Architecture/Replication/consistency0.png
[consistency1]:images/Distributed_Engine/Architecture/Replication/consistency1.png
[consistency2]:images/Distributed_Engine/Architecture/Replication/consistency2.png
[consistency_alive]:images/Distributed_Engine/Architecture/Replication/consistency_alive.png
[session_attr]: manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md
