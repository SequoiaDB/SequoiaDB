
配置快照可以列出数据库中指定节点的配置信息。

> **Note:**
>
> 每一个节点上的配置信息为一条记录。

##标识##

$SNAPSHOT_CONFIGS

##字段信息##

字段信息可参考[参数说明][configuration_parameters]。

##快照参数##

用户可以使用 use_option 指定快照参数。

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| Mode   | string  | 指定返回配置的模式，默认为“run” <br>在“run”模式下，显示当前运行时配置信息，在“local”模式下，显示配置文件中配置信息，如 { "Mode": "local" } | 否 |
| Expand | boolean/string  | 是否扩展显示用户未配置的配置项，默认为 true，如 { "Expand": false }| 否 |

##示例##

- 查看数据组 db1 中数据节点 20000 上的配置信息

   ```lang-javascript
   > db.exec( "select * from $SNAPSHOT_CONFIGS where GroupName = 'db1' and SvcName = '20000'" )
   ```

   输出结果如下：
   
   ```lang-json
   {
     "NodeName": "hostname:20000",
     "confpath": "/opt/trunk/conf/local/20000/",
     "dbpath": "/opt/test/20000/",
     "indexpath": "/opt/test/20000/",
     "diagpath": "/opt/test/20000/diaglog/",
     "auditpath": "/opt/test/20000/diaglog/",
     "logpath": "/opt/test/20000/replicalog/",
     "bkuppath": "/opt/test/20000/bakfile/",
     "wwwpath": "/opt/trunk/web/",
     "lobpath": "/opt/test/20000/",
     "lobmetapath": "/opt/test/20000/",
     "maxpool": 50,
     "diagnum": 20,
     "auditnum": 20,
     "auditmask": "SYSTEM|DDL|DCL",
     "svcname": "20000",
     "replname": "20001",
     "catalogname": "20003",
     "shardname": "20002",
     "httpname": "20004",
     "omname": "20005",
     "diaglevel": 3,
     "role": "data",
     "logfilesz": 64,
     "logfilenum": 20,
     "logbuffsize": 1024,
     "numpreload": 0,
     "maxprefpool": 0,
     "maxsubquery": 0,
     "maxreplsync": 10,
     "replbucketsize": 32,
     "syncstrategy": "KeepNormal",
     "preferredinstance": "M",
     "preferredinstancemode": "random",
     "preferredstrict": "FALSE",
     "instanceid": 0,
     "dataerrorop": 1,
     "memdebug": "FALSE",
     "memdebugsize": 0,
     "indexscanstep": 100,
     "dpslocal": "FALSE",
     "traceon": "FALSE",
     "tracebufsz": 256,
     "transactionon": "TRUE",
     "transactiontimeout": 60,
     "transisolation": 0,
     "translockwait": "FALSE",
     "transautocommit": "FALSE",
     "transautorollback": "TRUE",
     "transuserbs": "TRUE",
     "translrbinit": 524288,
     "sharingbreak": 7000,
     "startshifttime": 600,
     "catalogaddr": "hostname:30003,hostname:30013,hostname:30023",
     "tmppath": "/opt/test/20000/tmp/",
     "sortbuf": 256,
     "hjbuf": 128,
     "directioinlob": "FALSE",
     "sparsefile": "FALSE",
     "weight": 10,
     "auth": "TRUE",
     "planbuckets": 500,
     "optimeout": 300000,
     "overflowratio": 12,
     "extendthreshold": 32,
     "signalinterval": 0,
     "maxcachesize": 0,
     "maxcachejob": 10,
     "maxsyncjob": 10,
     "syncinterval": 10000,
     "syncrecordnum": 0,
     "syncdeep": "FALSE",
     "archiveon": "FALSE",
     "archivecompresson": "TRUE",
     "archivepath": "/opt/test/20000/archivelog/",
     "archivetimeout": 600,
     "archiveexpired": 240,
     "archivequota": 10,
     "omaddr": "",
     "dmschkinterval": 0,
     "cachemergesz": 0,
     "pagealloctimeout": 0,
     "perfstat": "FALSE",
     "optcostthreshold": 20,
     "enablemixcmp": "FALSE",
     "plancachelevel": 3,
     "maxconn": 0,
     "svcscheduler": 0,
     "svcmaxconcurrency": 0,
     "logwritemod": "increment",
     "logtimeon": "FALSE",
     "enablesleep": "FALSE",
     "recyclerecord": "FALSE",
     "indexcoveron": "TRUE",
     "maxsocketpernode": 1,
     "maxsocketperthread": 0,
     "maxsocketthread": 1,
     "businessname": "yyy",
     "clustername": "xxx"
   }
   ```

- 查看数据组 db1 中数据节点 20000 上的用户指定的配置信息

   ```lang-javascript
   > db.exec('select * from $SNAPSHOT_CONFIGS where GroupName = "db1" and SvcName = "20000" /*+use_option(Mode, local)use_option(Expand, false)*/') 
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "hostname:20000",
     "dbpath": "/opt/test/20000/",
     "svcname": "20000",
     "diaglevel": 3,
     "role": "data",
     "catalogaddr": "hostname:30003,hostname:30013,hostname:30023",
     "sparsefile": "TRUE",
     "plancachelevel": 3,
     "businessname": "yyy",
     "clustername": "xxx"
   }
   ```

[^_^]:
    本文使用的所有引用及链接
[configuration_parameters]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
