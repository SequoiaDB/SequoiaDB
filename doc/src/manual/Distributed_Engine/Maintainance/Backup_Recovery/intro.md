[^_^]:
    备份恢复原理
    作者：陈子川
    时间：20190303
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190523


本文档将介绍 SequoiaDB 巨杉数据库的全量备份、增量备份和恢复功能的原理。

全量备份原理
----

SequoiaDB 的全量备份功能，是将集群中指定数据分区的主节点所包含数据文件按照用户指定的方式，压缩保存在备份路径下。

SequoiaDB 的集群是由若干个数据分区组成，每个数据分区又可能存在多个副本节点。每个数据库引擎节点的 `dbpath` 目录通常如下：

```lang-bash
[sdbadmin@localhost 11830]$ ll -h
总用量 1.1G
drwxr-xr-x. 2 sdbadmin sdbadmin_group    6 12月  5 14:14 archivelog
drwxr-xr-x. 2 sdbadmin sdbadmin_group  129 2月  14 15:16 bak
drwxr-xr-x. 2 sdbadmin sdbadmin_group    6 12月  5 14:14 bakfile
drwxr-xr-x. 2 sdbadmin sdbadmin_group   45 2月   9 14:39 diaglog
-rw-r-----. 1 sdbadmin sdbadmin_group 149M 3月   2 15:05 sample.1.data
-rw-r-----. 1 sdbadmin sdbadmin_group 145M 3月   2 15:05 sample.1.idx
-rw-r-----. 1 sdbadmin sdbadmin_group 129M 2月  14 15:26 sample.1.lobd
-rw-r-----. 1 sdbadmin sdbadmin_group  81M 3月   2 15:05 sample.1.lobm
drwxr-xr-x. 2 sdbadmin sdbadmin_group 4.0K 12月  5 14:15 replicalog
-rw-r-----. 1 sdbadmin sdbadmin_group 149M 3月   2 15:05 SYSSTAT.1.data
-rw-r-----. 1 sdbadmin sdbadmin_group 145M 3月   2 15:05 SYSSTAT.1.idx
-rw-r-----. 1 sdbadmin sdbadmin_group  21M 3月   3 18:11 SYSTEMP.1.data
-rw-r-----. 1 sdbadmin sdbadmin_group  17M 3月   3 18:11 SYSTEMP.1.idx
-rw-r-----. 1 sdbadmin sdbadmin_group 149M 3月   2 15:05 test.1.data
-rw-r-----. 1 sdbadmin sdbadmin_group 145M 3月   2 15:05 test.1.idx
drwxr-xr-x. 2 sdbadmin sdbadmin_group    6 12月  5 14:14 tmp
```

全量备份功能，将对数据分区主节点的所有数据和索引数据进行备份。

> **Note:**
>
> 用户在对某数据分区执行全量备份时，该数据分区只能够提供数据查询服务。

增量备份原理
----

SequoiaDB 的增量备份功能，是对数据库集群中指定数据分区的主节点的同步日志进行日志解析后，按照数据库定义的格式，将新增同步日志打包保存在备份路径下。用户执行增量备份之前，需要确保该节点已经存在至少一次全量备份。

由于增量备份功能的实现原理是将引擎节点的同步日志新增部分处理后打包归档，所以用户需要确保两次相邻的增量备份操作时间间隔内，最老的同步日志没有被覆盖，否则增量备份操作将会失败。

> **Note**:
>
> 增量备份不阻塞数据分区的数据库读写服务。

恢复原理
----
SequoiaDB 的备份文件恢复原理是利用 [sdbrestore][sdbrestore] 工具将之前的全量备份文件和增量备份文件按照既定格式，重新解压后，恢复成正常的数据文件。



[^_^]:
    TODO:该页面需要调整
    设置 sdbrestore 参数
    设置 replica_log 参数



[^_^]:
    本文使用到的所有链接及引用。
[sdbrestore]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_restore.md

