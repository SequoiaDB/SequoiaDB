[^_^]:
    日志重放工具
    

sdbreplay 是 SequoiaDB 巨杉数据库的日志重放工具，用于重放已经[归档][log_archive]的同步日志。用户可以在其他集群或节点重复执行日志重放工具，以实现不同集群间的数据同步。

##基本功能##

日志重放工具主要功能如下:

- 读取日志并在 SequoiaDB 巨杉数据库上执行重放
- 根据条件（日志文件、集合空间、集合、操作和 LSN）过滤日志进行重放
- 持续监控归档目录并重放日志
- 在后台执行日志重放
- 生成状态文件，重启后继续执行退出前的重放

支持重放的日志操作如下：

| 操作       | 说明         |
| ---------- | ------------ |
| insert     | 插入数据     |
| update     | 更新数据     |
| delete     | 删除数据     |
| truncatecl | truncate 集合 |

>**Note:**
>
>- 重放时根据归档文件的 FileId 从小到大重放
>- 重放出错时重放工具会立即退出，已经重放的日志不会回滚
>- 归档日志 LSN 不连续时重放工具会报错退出
>- 通过 status 选项指定状态文件，可以从上次退出的地方继续重放，不指定时每次从第一个归档文件开始重放
>- 未重放的归档日志文件发生了移动操作后，移动文件不会被重放；已重放的归档日志文件发生了移动操作，重放工具会回滚移动文件中的日志操作
>- 不支持回滚 truncatecl 操作，回滚时遇到 truncatecl 操作会报错退出
>- 在后台执行时可以通过 kill -15 的方式停止进程
>- 复制日志是幂等的，同一条日志多次重放结果不变
>- 归档重放过程中，如果在集合上执行 split 操作，通过协调节点重放到同一个集群时可能会丢失数据。因为 split 的源复制组在数据迁移到目标复制组后会删除本地相应数据，并生成删除日志；而目标复制组接收数据后生成插入日志。如果目标复制组的归档日志先被重放而后源复制组的归档日志才被重放，那么迁移的那部分数据重放时先插入后被删除，导致数据丢失。此时可以通过再次重放目标复制组的归档日志来重新插入丢失的数据

##参数说明##

| 参数名        | 缩写 | 描述 |
| ----          | ---- | ---- |
| --help        | -h   | 打印帮助信息                        |
| --version     | -V   | 打印版本号                          |
| --hostname    |      | SequoiaDB 所在的主机名，--dump 和 --dumpheader 为 false，且 --outputconf 未设置时必填                          |
| --svcname     |      | SequoiaDB 的服务名（端口号），--dump 和 --dumpheader 为 false，且 --outputconf 未设置时必填                          |
| --user        |      | 数据库用户                          |
| --password    |      | 数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码                                       |
| --cipher      |      | 是否使用密文模式输入密码，默认值为 false，不使用密文模式输入密码 <br> 关于密文模式介绍，可参考[密码管理][passwd_mgm] |
| --token       |      | 密文文件的加密令牌<br>如果创建密文文件时未指定 token，可忽略该参数                |
| --cipherfile  |      | 加密文件路径，默认路径为 `~/sequoiadb/passwd`                                                       |
| --ssl         |      | 是否使用 SSL 连接，默认值为 false，不使用 SSL 连接                                                                   |
| --path        |      | 归档目录，可以是文件或目录，必填    |
| --outputconf  |      | 用于配置回放工具的输出规则，当设置 --hostname 时该参数无效，该参数详细说明可参考 [outputconf说明][outputconf]                                                      |
| --filter      |      | 过滤条件                            |
| --dump        |      | 是否打印归档日志信息，默认值为 false，取值如下：<br>true：只打印日志信息，不执行重放操作<br>false：执行重放操作，不打印日志信息|
| --dumpheader  |      | 是否打印归档日志元数据的头信息，默认值为 false，取值如下：<br>true：只打印日志元数据的头信息，不执行重放操作<br>false：执行重放操作，不打印日志元数据的头信息|
| --delete      |      | 是否删除已完成重放的完整归档日志文件，默认值为 false，取值如下：<br>true：删除 <br>false：不删除            |
| --watch       |      | 是否持续监控归档目录并重放日志，设置 --path 为目录时该参数有效，默认值为 false，不持续监控                             |
| --daemon      |      | 是否在后台运行，默认值为 false，取值如下：<br>true：在后台运行，执行 `kill -15 <pid>` 可以使后台进程正确退出<br>false：不在后台运行              |
| --status      |      | 指定状态文件，状态文件会存储重放的状态信息，首次指定时重放工具会生成该文件；重放工具退出后，通过指定状态文件可以从上次退出的地方继续重放 |
| --intervalnum |      | 状态文件持久化间隔记录数，每回放 intervalnum 条记录持久化一次状态文件，默认值为 1000 |
| --type        |      | 指定日志类型，默认值为“archive” <br> “archive”：归档日志<br>“replica”：复制日志 |

其中 --filter 为 json 格式的字符串，可以指定过滤条件对日志进行过滤，支持的过滤条件如下：

| 字段     | 类型          | 描述                                             |
| ----     | ----          | ----                                             |
| OP       | array[string] | 指定重放的操作，默认为重放工具支持的所有日志操作 |
| ExclOP   | array[string] | 指定排除的操作，优先级高于OP                     |
| MinLSN   | int32         | 指定重放的最小 LSN（包括该 LSN）                 |
| MaxLSN   | int32         | 指定重放的最大LSN（不包括该LSN）                 |
| File     | array[string] | 指定重放的日志文件，默认为全部                   |
| ExclFile | array[string] | 指定排除的日志文件，优先级高于File               |
| CS       | array[string] | 指定重放的集合空间，默认为全部                   |
| ExclCS   | array[string] | 指定排除的集合空间，优先级高于CS                 |
| CL       | array[string] | 指定重放的集合，集合名的格式为"集合空间.集合名"，默认为全部 |
| ExclCL   | array[string] | 指定排除的集合，优先级高于CL                     |

##outputconf 说明##

outputconf 是以 json 格式表示的，用于设置输出格式的配置文件，其具体参数有：

|参数名|类型|描述|
|----  |----|----|
|outputType|string|目标格式类型，当前只有一种类型：DB2LOAD|
|outputDir|string|输出目录|
|filePrefix|string|输出结果文件名的前缀|
|fileSuffix|string|输出结果文件名的后缀，与 filePrefix 至少配置一个，该后缀在文件类型前面|
|delimiter|string|字段分隔符，默认为','|
|submitTime|string|文件提交时间，格式为 21:00 ，表示 21：00 提交一次正式文件|
|submitInterval|int64|文件提交的时间间隔，单位为分钟，每隔指定的分钟数提交一次文件；当配置了 submitInterval 后，submitTime 不生效|
|tables|array[obj]|表映射关系|
|tables.source|string|源表名|
|tables.target|string|目标表名|
|fields|array[obj]|字段映射关系|
|fields.fieldType|string|字段类型，取值如下：<br> "ORIGINAL_TIME"：复制日志生成时间 <br> "AUTO_OP"：操作标识串，如 I:insert、D:delete、B:before update、A:after update <br> "CONST_STRING"：常量字符串 <br> "MAPPING_STRING"：映射原始记录中的字符串 <br> "MAPPING_INT"：映射原始记录中的int类型字段 <br> "MAPPING_LONG"：映射原始记录中的long类型字段 <br> "MAPPING_DECIMAL"：映射原始记录中的 decimal 类型字段 <br> "MAPPING_TIMESTAMP"：映射原始记录中的时间戳字段 |
|fields.doubleQuote|boolean|value 是否带双引号，默认为 true|
|fields.constValue|string|fieldType 为"CONST_STRING"时使用，表示常量的值|
|fields.source|string|源字段名|
|fields.target|string|目标字段名|
|fields.defaultValue|string|默认值，当源字段名不存在时，输出默认值，而不是报错；仅支持映射字段（以 MAPPING_ 开头的字段类型）|

outputconf 示例如下：

```lang-json
$cat output.conf
{
  outputType: "DB2LOAD",
  outputDir: "/home/mount/sequoiadb/replay/output/",
  filePrefix: "SDB_db1_1000",
  submitTime: "21:00",
  delimiter: ",",
  tables:
  [
   {
     source: "cs.cl",
     target: "dbName.tableName",
     fields:
     [
       {
         fieldType: "ORIGINAL_TIME"
       },
       {
         fieldType: "CONST_STRING",
         constValue: "0"
       },
       {
         fieldType: "AUTO_OP"
       },
       {
         source: "a",
         target: "column1",
         fieldType: "MAPPING_STRING"
       },
       {
         source: "b",
         target: "column2",
         fieldType: "MAPPING_STRING",
         doubleQuote: false
       },
       {
         source: "_id",
         target: "column3",
         fieldType: "MAPPING_STRING"
       }
     ]
   }
  ]
}
```

上述配置下，生成的结果文件格式如下：

```lang-bash
$cat SDB_db1_1000_dbName_tableName_0000000001_384_201904291212.csv
"2019-04-10 14.52.17.551928","0","D","a4",b4,"5cad8cc8da342dfe37a40e84"
"2019-04-10 14.52.17.553750","0","I","a1",b1111,"5cac3850da342dfe37a40eee"
"2019-04-10 14.52.17.553820","0","B","a1",b1111,"5cac3850da342dfe37a40eee"
"2019-04-10 14.52.17.553820","0","A","a1",b22,"5cac3850da342dfe37a40eee"
```

>**Note:**
>
> 第一条为删除操作，删除记录{"_id": {"$oid": "5cad8cc8da342dfe37a40e84"}, "a": "a4", "b": "b4"}
>
> 第二条为插入操作，插入记录{"a": "a1", "b": "b1111"}
>
> 第三、四条为更新操作，更新前记录为：{"_id": {"$oid": "5cac3850da342dfe37a40eee"}, "a": "a1", "b": "b1111"}，更新后操作为：{"_id": {"$oid": "5cac3850da342dfe37a40eee"}, "a": "a1", "b": "b22"}

##示例##

- 指定归档目录下的 `archivelog.1` 日志文件进行重放

   ```lang-bash
   $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog/archivelog.1
   ```
- 指定归档目录并过滤集合 sample.employee 的 insert 和 update 操作进行重放

   ```lang-bash
   $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog --filter '{ "CL": [ "sample.employee" ], "OP": ["insert","update"] }'
   ```
   
- 在后台持续监控归档目录并重放归档日志文件，同时记录状态

   ```lang-bash
   $./sdbreplay --hostname sdbserver1 --svcname 11810 --path /data/archivelog --watch true --daemon true --status 1.status
   ```

   [^_^]:
    本文使用到的所有链接及引用。
[log_archive]:manual/Distributed_Engine/Maintainance/Backup_Recovery/log_archive.md
[passwd_mgm]: manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理
[outputconf]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/log_replay.md#outputconf说明