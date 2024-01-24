[^_^]:
    节点诊断日志
    作者：黎锐昌
    时间：20190703
    时间：20190812
    评审意见
    王涛：
    许建辉：20191122


SequoiaDB 巨杉数据库的节点诊断日志记录了数据库节点执行过的操作信息。通过节点诊断日志，用户可以对数据库节点的运行状态进行故障分析、行为分析等操作，能有效帮助用户获取数据库节点的执行情况。

节点诊断日志路径
----

节点诊断日志默认保存在 `节点数据目录/diaglog/` 目录下。节点当前正在使用的诊断日志名为 `sdbdiag.log`，归档的诊断日志名为 `sdbdiag.log.日志归档时间戳`。

### 节点数据目录

节点数据目录可通过执行 `数据库安装路径/bin/sdblist` 命令查看 DBPath 参数的方式获取，sdblist 命令详解可参考 `数据库安装路径/bin/sdblist --help` 帮助信息。

**示例**

1. 查看数据节点 11830 的数据目录

   ```lang-bash
   $ sdblist -l -p 11830
   ```

   返回结果如下：

   ```lang-text
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11830         data        1522      1000   1000   Y    group1               2019-08-12-05.18.53  /sequoiadb/database/data/11830/
   ```

2. 查看数据节点 11830 的诊断日志

   ```lang-bash
   $ more /sequoiadb/database/data/11830/diaglog/sdbdiag.log
   ```

节点诊断日志解读
----

查看节点诊断日志返回部分结果如下：

```lang-text
2019-07-03-07.02.54.395935               Level:ERROR
PID:1391                                 TID:2830
Function:execute                         Line:1326
File:SequoiaDB/engine/coord/coordCommandData.cpp
Message:
Failed to create collection space[employees], rc: -33
```

节点诊断日志字段说明

| 字段     | 说明                                   |
| -------- | -------------------------------------- |
| Level    | 日志级别                               |
| PID      | 进程号                                 |
| TID      | 线程号                                 |
| Function | 函数名，当前操作对应的内部函数名       |
| Line     | 函数中行号，当前日志对应的函数中的行号 |
| File     | 函数源文件                             |
| Message  | 详细信息                               |

> **Note：**
>
> Message 中“rc: -33”为执行失败返回的[错误码][errorCode]。

节点诊断日志归档
----

当节点诊断日志文件记录满 100MB 时，诊断日志文件会被归档保存。当归档的诊断日志文件数量等于 diagnum 所设置的最大数量时，日志文件将被循环使用，新的日志会覆盖旧的日志。诊断日志文件最大数量默认为 20，用户可根据实际需要修改。

**示例**

1. 执行 [updateConf][updateConf] 命令，修改 11830 数据节点的诊断日志文件最大数量为不限制并动态生效

   ```lang-javascript
   db.updateConf({diagnum:-1},{Svcname:"11830"})
   ```

2. 查看 11830 数据节点的配置信息

   ```lang-bash
   sdblist --expand -p 11830 | grep diagnum
   ```

   结果返回以下信息，说明配置修改成功

   ```lang-text
   diagnum           : -1
   ```

[^_^]: 
    如下链接没有，后面补充

[errorCode]:manual/Manual/Sequoiadb_error_code.md
[updateConf]: manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
