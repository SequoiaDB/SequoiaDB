[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190320
    评审意见

    王涛：
    许建辉：
    市场部：20190425


配置快照列出数据库中指定节点的配置信息。

标识
----

SDB_SNAP_CONFIGS

字段信息
----

字段信息详见[数据库配置][runtime_config_url]。

扩展参数
----

构造扩展参数，可参考 [SdbSnapshotOption][sdbsnapshotoption_url]。

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| Mode   |	string  | 指定返回配置的模式，默认为"run"<br>在 run 模式下，显示当前运行时配置信息；在 local 模式下，显示配置文件中配置信息 | 否 |
| Expand |	boolean    | 是否扩展显示用户未配置的配置项，默认为 true | 否 |

示例 
----

- 构造扩展参数进行快照查询

   ```lang-javascript
   > var option = new SdbSnapshotOption().options( { "Mode": "local", "Expand": false } )
   > db.snapshot( SDB_SNAP_CONFIGS, option )
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver1:11830",
     "dbpath": "/opt/sequoiadb/database/data/11830/",
     "svcname": "11830",
     "role": "data",
     "catalogaddr": "sdbserver1:11823,sdbserver2:11823,sdbserver3:11823",
     "omaddr": "sdbserver1:11785",
     "businessname": "myService1",
     "clustername": "myCluster1"
   }
   ...
   ```

- 指定服务名和组名进行快照查询

   ```lang-javascript
   > db.snapshot( SDB_SNAP_CONFIGS, { GroupName:'group1', SvcName:'11830' } )
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver1:11830",
     "confpath": "/opt/sequoiadb/conf/local/11830/",
     "dbpath": "/opt/sequoiadb/database/data/11830/",
     "indexpath": "/opt/sequoiadb/database/data/11830/",
     "diagpath": "/opt/sequoiadb/database/data/11830/diaglog/",
     "auditpath": "/opt/sequoiadb/database/data/11830/diaglog/",
     "logpath": "/opt/sequoiadb/database/data/11830/replicalog/",
     "bkuppath": "/opt/sequoiadb/database/data/11830/bakfile/",
     "wwwpath": "/opt/sequoiadb/web/",
     "lobpath": "/opt/sequoiadb/database/data/11830/",
     "lobmetapath": "/opt/sequoiadb/database/data/11830/",
     "maxpool": 50,
     "diagnum": 20,
     "auditnum": 20,
     "auditmask": "SYSTEM|DDL|DCL",
     "svcname": "11830",
     "replname": "11831",
     "catalogname": "11833",
     "shardname": "11832",
     "httpname": "11834",
     "omname": "11835",
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
     "instanceid": 0,
     "dataerrorop": 1,
     "memdebug": "FALSE",
     "memdebugsize": 0,
     "indexscanstep": 100,
     "dpslocal": "FALSE",
     "traceon": "FALSE",
     "tracebufsz": 256,
     "transactionon": "FALSE",
     "transactiontimeout": 60,
     "transisolation": 0,
     "translockwait": "FALSE",
     "translrbinit": 524288,
     "translrbtotal": 268435456,
     "sharingbreak": 7000,
     "startshifttime": 600,
     "catalogaddr": "sdbserver1:11823,sdbserver2:11823,sdbserver3:11823",
     "tmppath": "/opt/sequoiadb/database/data/11830/tmp/",
     "sortbuf": 256,
     "hjbuf": 128,
     "directioinlob": "FALSE",
     "sparsefile": "FALSE",
     "weight": 10,
     "usessl": "FALSE",
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
     "archivepath": "/opt/sequoiadb/database/data/11830/archivelog/",
     "archivetimeout": 600,
     "archiveexpired": 240,
     "archivequota": 10,
     "omaddr": "sdbserver1:11785",
     "dmschkinterval": 0,
     "cachemergesz": 0,
     "pagealloctimeout": 0,
     "perfstat": "FALSE",
     "optcostthreshold": 20,
     "enablemixcmp": "FALSE",
     "plancachelevel": 2,
     "maxconn": 0,
     "svcscheduler": 0,
     "svcmaxconcurrency": 0,
     "businessname": "myService1",
     "clustername": "myCluster1",
     "usertag": ""
   }
   ...
   ```

[^_^]:
    本文使用到的所有链接及引用。
    
[runtime_config_url]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[sdbsnapshotoption_url]: manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
