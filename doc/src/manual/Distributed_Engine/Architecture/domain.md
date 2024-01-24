数据域（Domain）是指由若干个复制组组成的逻辑单元，其主要作用是数据隔离。

- 一个复制组可以属于多个域。
- 复制组个数为 0 的域，称作空域。用户在空域中不能创建集合空间。
- “SYSDOMAIN”为预定义的系统域。所有复制组均属于系统域，用户不能直接操作系统域。

原理
----

![Domain示意图][domain]

- 图中共有6个复制组：prod_part1，prod_part2，prod_part3，prod_part4，prod_part5，prod_part6
- 一共有4个数据域：prod_domain1，prod_domain2，prod_domain3 和 SYSDOMAIN。数据域 prod_domain1 内只有一个复制组 prod_part1；数据域 prod_domain2 内有两个复制组 prod_part2 和 prod_part3；数据域 prod_domain3 内有两个复制组 prod_part3，prod_part6；SYSDOMAIN 域包含所有的6个复制组
- 复制组 prod_part3 同时属于数据域 prod_domain2，prod_domain3，SYSDOMAIN。

在创建集合空间时，可以指定该集合空间所属的数据域。后续该集合空间下的集合只能创建在数据域包含的数据组内，不能在其他数据组上，从而实现的数据隔离。

数据隔离操作实例
----

通过使用不同的数据域，来实现不同业务的数据隔离。以用户需要将两个业务（业务 A，业务 B）做数据隔离为例，说明使用数据域实现数据隔离的操作方法。

- 创建两个包含不同复制组的域

  ```lang-bash
  > db.createDomain( 'prod_domain1', ['prod_part1', 'prod_part2'], { AutoSplit: true } )
  > db.createDomain( 'prod_domain2', ['prod_part3', 'prod_part4'], { AutoSplit: true } )
  ```

- 创建业务 A 需要的集合空间和集合

  ```lang-bash
  > db.createCS( 'businessA', { Domain: 'prod_domain1' } )
  > db.businessA.createCL( 'orders' )
  ```

- 创建业务 B 需要的集合空间和集合

  ```lang-bash
  > db.createCS( 'businessB', { Domain: 'prod_domain2' } )
  > db.businessB.createCL( 'orders' )
  ```

- 集合 businessA.orders 的数据只会落在数据域 prod_domain1 中，对应的复制组为 prod_part1，prod_part2；集合 businessB.orders 的数据只会落在数据域 prod_domain2 中，对应的复制组为 prod_part3，prod_part4；

数据自动切分操作实例
----

数据域的另一个作用是在使用散列分区方式做数据库分区时，实现自动切分。当创建集合时，指定散列分区和 AutoSplit 参数时，集合的数据将会自动切分到集合所在域的所有复制组中。

- 创建域

  ```lang-bash
  > db.createDomain( 'prod_domain3', ['prod_part1', 'prod_part2'], { AutoSplit: true } )
  ```

- 创建集合空间，并指定集合空间所属的域

  ```lang-bash
  > db.createCS( 'business', { Domain: 'prod_domain3' } )
  ```

- 创建集合，指定散列分区和 AutoSplit

  ```lang-bash
  > db.business.createCL( 'orders', { ShardingType: 'hash', ShardingKey: { id: 1 }, AutoSplit: true } )
  ```

+ 通过[快照命令][SDB_SNAP_CATALOG]可以看到集合 business.orders 自动在域内所有复制组中进行了 hash 切分

  ```lang-bash
  > db.snapshot( SDB_SNAP_CATALOG, { Name: 'business.orders'} )
  ```

  输出结果如下：

  ```lang-json
  {
    ...
    "Name": "business.orders",
    "ShardingType": "hash",
    "ShardingKey": {
      "id": 1
    }
    "CataInfo": [
      {
        "ID": 0,
        "GroupdID": 1000,
        "GroupName": "prod_part1",
        "LowBound": {
          "": 0
        },
        "UpBound": {
          "": 2048
        }
      },
      {
        "ID": 1,
        "GroupdID": 1001,
        "GroupName": "prod_part2",
        "LowBound": {
          "": 2048
        },
        "UpBound": {
          "": 4096
        }
      }
    ]
  }
  ```

  数据库自动将集合 business.orders 在复制组 prod_part1 和 prod_part2 中做了切分。将字段 id 的 hash 值范围在[0, 2048)的数据划分到复制组 prod_part1 中，字段 id 的 hash 值范围在[2048, 4096)的数据划分到复制组 prod_part2 中。


[^_^]:
      本文使用的所有引用和链接
[domain]:images/Distributed_Engine/Architecture/domain.png
[SDB_SNAP_CATALOG]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md
