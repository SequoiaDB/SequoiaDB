[^_^]:
    数据恢复工具
    

sdbrevert 是 SequoiaDB 巨杉数据库的数据恢复工具，用于从[同步日志][log_replica]、[归档日志][log_archive]中恢复被误删除的记录、lob 数据。

##基本功能##

数据恢复步骤：

1. 明确待恢复数据的范围，如目标集合、误操作的时间范围、数据匹配条件等
2. 使用 sdbrevert 工具，根据指定的数据恢复条件，从目标日志文件中，读取、解析数据并将删除的记录、lob 数据写入指定的临时集合中
3. 人工复核临时集合中的数据，挑选出误删除的数据，并自行将其恢复至正式集合中

支持恢复的数据如下：

| 类型       | 说明         |
| ---------- | ------------ |
| delete doc | 记录数据误删除 |
| delete lob | lob 数据误删除 |

>**Note:**
>
>- 3.4.2 及以后版本 SequoiaDB 的日志才支持恢复 lob 数据
>- 不支持发生移动的归档日志文件，如 archivelog.1.m


##参数说明##

| 参数名        | 缩写 | 描述 |
| ----          | ---- | ---- |
| --help        | -h   | 打印帮助信息                        |
| --version     | -V   | 打印版本号                          |
| --hostname    |      | 主机名                              |
| --svcname     |      | 服务名（端口号）                    |
| --hosts       |      | 指定主机地址（hostname:svcname），用“,”分隔多个地址，默认值为 localhost:11810|
| --user        |      | 数据库用户                          |
| --password    |      | 数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码                                       |
| --token       |      | 密文的加密令牌<br>如果创建密文时未指定 token，可忽略该参数 |
| --cipherfile  |      | 加密文件路径，默认路径为 `~/sequoiadb/passwd`。关于密文模式可参考[密码管理][passwd_mgm] |
| --ssl         |      | 是否使用 SSL 连接，默认值为 false，不使用 SSL 连接 |
| --targetcl    |      | 恢复数据的目标集合 (csname.clname)，必填 |
| --logpath     |      | 日志路径，用“,”分隔多个文件、路径按，支持复制日志、归档日志，必填    |
| --datatype    |      | 恢复的数据类型，默认为 "all"，取值如下：<br>all：同时恢复记录与 lob 数据 <br>doc：仅恢复记录数据 <br>lob：仅恢复 lob 数据 |
| --matcher     |      | 数据过滤条件，json 格式 |
| --starttime   |      | 起始时间，格式 “YYYY-MM-DD-HH:mm:ss”，默认为 UTC 时间 1970-01-01-00:00:00 |
| --endtime     |      | 结束时间，格式 “YYYY-MM-DD-HH:mm:ss”，默认为 2037-12-31-23:59:59 |
| --startlsn    |      | 起始 LSN，默认为 0 |
| --endlsn      |      | 结束 LSN，默认不限制 |
| --output      |      | 保存数据的临时集合 (csname.clname)，必填 |
| --label       |      | 标签，供用户标识不同批次恢复的记录数据。默认为 "" |
| --jobs        |      | 执行数据恢复的线程数量，默认为 1。多线程以日志文件为单位并发恢复数据 |
| --stop        |      | 单个线程执行失败时，其余线程是否需要停止执行。默认为 false，取值如下：<br>false：其余线程继续执行 <br>true：其余线程停止执行 |
| --fillhole    |      | 是否以 0 填充 lob 数据的空洞。默认为 false，取值如下：<br>false：不提填充，保留 lob 空洞 <br>true：填充 |
| --tmppath     |      | 临时路径，用于保存解压归档日志时产生的临时文件，默认保存在归档日志文件所属目录 |

>**Note:**
>
>- 恢复数据时，同一复制组内，仅需指定一个副本节点的日志，以免恢复的数据重复
>- lob 数据以分片的形式分散存储在多个复制组中。在恢复 lob 数据时，需要确保指定的日志文件包含了完整的 lob 数据，以免恢复的 lob 数据不完整
>- 如果多次创建、删除了相同 oid 的 lob 数据。在恢复该 oid 的 lob 数据时，以最新一次删除操作为准，所以最终恢复的结果不保证准确性。
>- 时间范围为 [starttime, endtime)，且是根据日志文件的最后一次修改时间进行过滤。最大时间范围 [1970-01-01-00:00:00, 2037-12-31-23:59:59)
>- LSN 范围为 [startlsn, endlsn)
>- 临时集合需要用户自行创建，且 sdbrevert 不会清除临时集合中的历史数据 
>- 对于 lob 数据，可写入的不连续分片不能超出 320 个，如果超出，则继续写该 lob 时会报 -319 错误。对于超大 lob，如 40MB 以上，sdbrevert 按分片恢复 lob 数据时，可能会出现写入的不连续 lob 分片超过 320 个。此时如果 --fillhole 取值为 false，则会跳过 -319 错误，最终恢复的超大 lob 数据不保证正确性。如果 --fillhole 取值为 true，则 sdbrevert 会以 0 填充 lob 空洞，以此合并不连续的 lob 分片，以免数量超出限制，最终保证恢复的 lob 的数据正确性，但该 lob 将不存在空洞
>- 如果归档日志开启了压缩功能，且 sdbrevert 对归档日志所在路径没有写权限时，需要指定 --tmppath 参数。解压产生的临时文件最终会被 sdbrevert 自动删除。

##结果汇总##

sdbrevert 执行完成后，会返回如下信息：

| 字段名                  | 描述                                   |
| ----------------------- | -------------------------------------- |
| Processed file num      | 实际处理的日志文件数量，仅统计恢复范围内的有效日志文件 |
| LSN scope               | 实际恢复的 LSN 范围 [startlsn, endlsn) |
| Parsed log num          | 解析的日志条数                         |
| Parsed doc num          | 解析被删除的记录数量                   |
| Parsed lob pieces num   | 解析被删除的 lob 分片数量              |
| Reverted doc num        | 恢复被删除的记录数量                   |
| Reverted lob pieces num | 恢复被删除的 lob 分片数量              |
| Revert failed log file  | 恢复失败的 lob 文件                    |

##使用示例##

从 11820 节点的 replicalog 中恢复 sample.employee 的记录、lob 数据至 revertCS.revertCL 集合

```lang-shell
sdbrevert --hosts localhost:11810 --targetcl sample.employee --logpath ./database/data/11820/replicalog/ --output revertCS.revertCL
```

恢复指定 oid 的记录数据。特殊字符 "$" 需要转义处理

```lang-shell
sdbrevert --hosts localhost:11810 --targetcl sample.employee --logpath ./database/data/11820/replicalog/ --output revertCS.revertCL --matcher "{'_id': {'\$oid': '649aad16936f54aba362ff61'}}" --datatype doc
```

恢复指定 oid 的 lob 数据。oid 中的 key 与记录数据一样是 "_id"

```lang-shell
sdbrevert --hosts localhost:11810 --targetcl sample.employee --logpath ./database/data/11820/replicalog/ --output revertCS.revertCL --matcher "{'_id': {'\$oid': '0000649aebfb3600042dd1e0'}}" --datatype lob
```


##恢复数据至正式集合##

###记录数据###

记录数据被封装为如下格式保存在临时集合中

| 字段名 | 描述                            |
| ------ | ------------------------------- | 
| entry  | 原始数据                        |
| optype | 操作类型，当前仅为 "DOC_DELETE" |
| label  | 自定义的标签                    |
| lsn    | 该记录在日志中的 LSN            |
| source | 记录所属的日志文件              |

示例

```lang-json
{
  "_id": {
    "$oid": "649aad2e23300bc3c3fc7e06"
  },
  "entry": {
    "_id": {
      "$oid": "649aad16936f54aba362ff61"
    },
    "name": "Tom",
    "age": 20
  },
  "optype": "DOC_DELETE",
  "label": "",
  "lsn": 393771196,
  "source": "/opt/sequoiadb/11810/replicalog/sequoiadbLog.2"
}
```

将记录数据恢复至正式集合中

```lang-js
var matcher = { "entry._id": { "$oid": "649aad16936f54aba362ff61" } };
var selector = { "entry": "" };

// 从临时集合 revertCS.revertCL 获取数据
var cursor = db.revertCS.revertCL.find( matcher, selector );
while( cursor.next() )
{
   // 将数据写入正式表 sample.employee 中
   db.sample.employee.insert( cursor.current().toObj()["entry"] );
}

cursor.close();
```

如果临时集合中的数据量较多，可根据原始数据创建非唯一索引，以提升处理效率

```lang-js
var idxName = "dataIndex";
var idxDef = { "entry._id": 1 };

db.revertCS.revertCL.createIndex( idxName, idxDef );
```

###lob 数据###

lob 数据以普通格式保存在临时集合中，可使用 [listLobs()][listLobs] 查看，示例如下

```lang-js
> db.revertCS.revertCL.listLobs();
{
  "Size": 22020096,
  "Oid": {
    "$oid": "0000649aebfb3600042dd1e0"
  },
  "CreateTime": {
    "$timestamp": "2023-06-27-14.05.02.552000"
  },
  "ModificationTime": {
    "$timestamp": "2023-06-27-14.05.02.724000"
  },
  "Available": true
}
```

将 lob 数据恢复至正式集合中

```lang-js
var tmpFile = "/opt/sequoiadb/tmp.txt";

var lobId = "0000649aebfb3600042dd1e0";
db.revertCS.revertCL.getLob( lobId, tmpFile );

db.sample.employee.putLob( tmpFile, lobId );
```


[^_^]:
    本文使用到的所有链接及引用。
[log_replica]:manual/Distributed_Engine/Architecture/Replication/architecture.md#同步日志
[log_archive]:manual/Distributed_Engine/Maintainance/Backup_Recovery/log_archive.md
[passwd_mgm]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理
[listLobs]:manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md