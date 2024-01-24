[^_^]:
    备份恢复Readme


在分布式数据库集群环境下，多副本机制可以有效避免集群单机的宕机风险，并提供高可用的容灾技术需求。而数据库备份功能，能够在用户误操作造成数据丢失后，帮助用户快速恢复原有数据库数据。

数据库备份功能，是对数据库现有数据进行备份。数据库备份功能可以帮助用户在以下场景快速恢复数据库原有数据：

* 数据库集群所有服务器损坏，并且无法修复
* 所有副本节点的磁盘数据损坏，并且无法恢复
* 用户误操作，导致删除或者修改了关键数据

通过本章文档，用户可以了解 SequoiaDB 巨杉数据库备份恢复的原理及使用。主要内容如下：

* [备份恢复原理][intro]
* [数据备份][data_backup]
* [日志归档][log_archive]
* [数据恢复][data_restore]



[^_^]:
    本文使用到的所有链接及引用。
[intro]:manual/Distributed_Engine/Maintainance/Backup_Recovery/intro.md
[data_backup]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md
[log_archive]:manual/Distributed_Engine/Maintainance/Backup_Recovery/log_archive.md
[data_restore]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_restore.md

