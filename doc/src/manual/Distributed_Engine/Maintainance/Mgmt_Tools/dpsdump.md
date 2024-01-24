[^_^]:
    数据库日志 dump 工具

sdbdpsdump 是 SequoiaDB 巨杉数据库的同步日志解析工具，用于解析目录 `replicalog` 下的同步日志内容。当集群数据存在异常时，可使用该工具进行数据分析。

##语法规则##

**sdbdpsdump [--source | -s arg] [--output | -o arg] [ --type | -t arg] [--name | -n arg] [--lsn | -l arg] [--ahead | -a arg] [--meta | -m] [--last | -e arg] [--back | -b arg] [--transaction arg]**

##参数说明##

| 参数名 | 缩写 | 描述 |
| ------ | ---- | ---- |
| --help    | -h | 获取帮助信息 |
| --version | -v | 获取版本信息 |
| --meta    | -m | 解析日志文件的元数据信息 |
| --type    | -t | 指定按操作类型解析日志文件，支持指定的类型如下：<br>● 1：数据插入<br>● 2：数据更新<br>● 3：数据删除<br>● 4：创建集合空间<br>● 5：删除集合空间<br>● 6：创建集合<br>● 7：删除集合<br>● 8：创建索引<br>● 9：删除索引<br>● 10：集合重命名<br>● 11：集合 truncate<br>● 12：事务提交<br>● 13：事务回滚<br>● 14：清空 Catalog 缓存<br>● 15：写入 LOB 数据 <br>● 16：删除 LOB 数据<br>● 17：修改 LOB 数据<br>● 21：修改集合/集合空间的属性<br>● 22：添加集合/集合空间的 UniqueID |
| --name    | -n | 指定集合空间或者集合名，表示仅解析与该集合空间或者集合相关的日志 |
| --lsn     | -l | 指定 LSN，表示仅解析与该 LSN 对应的日志<br>该参数可搭配 --ahead 和 --back 使用 |
| --last    | -e | 指定解析日志文件最后 N 条日志，N 为待指定的参数值<br>使用该参数时，需保证参数 --source 的值为具体的日志文件 |
| --transaction | - | 指定事务 ID，表示仅解析与该事务 ID 对应的日志<br>事务 ID 需按 16 进制格式指定，例如 `--transaction 0x000400667d033b` |
| --source | -s | 指定日志文件所在目录或路径，不指定该参数时默认为当前目录 |
| --output | -o | 指定输出文件，不指定该参数时默认为屏幕输出 |
| --ahead  | -a | 指定解析 LSN 之前的 N 条日志，N 为待指定的参数值，默认值为 20<br>该参数必须与 --lsn 配合使用 |
| --back   | -b | 指定解析 LSN 之后的 N 条日志，N 为待指定的参数值，默认值为 20<br>该参数必须与 --lsn 配合使用 |

##常见场景##

- 解析日志文件的元数据信息，并将结果输出至文件 `meta.log`

    ```lang-bash
    $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/ -m -o meta.log
    ```

    文件的元数据信息如下，用户可通过该信息了解各个文件对应数据的范围：

    ```lang-text
    =======================================
        Log Files in total: 20
        LogFile begin     : sequoiadbLog.0
        LogFile work      : sequoiadbLog.0
        Begin Lsn         : 0x00000000
        Current Lsn       : 0x00000644
        Expect Lsn        : 0x00000710
    =======================================
    
    Log File Name: sequoiadbLog.0
    Logic ID     : 0
    First LSN    : 0x00000000
    Last  LSN    : 0x00000644
    Valid Size   : 1808 bytes
    Rest Size    : 67107056 bytes
    ...
    ```

    上述字段说明如下：

    | 字段名 | 描述 |
    | ------ | ---- |
    | LogFile begin | 起始日志文件 |
    | LogFile work | 结束日志文件 |
    | Frist LSN | 第一条日志对应的 LSN |
    | Last LSN | 最后一条日志对应的 LSN |
    | Valid Size | 日志已用空间 |
    | Rest Size | 日志空闲空间 |

- 解析数据插入类型的日志，并将结果输出至文件 `out.log`

    ```lang-bash
    $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/ -o out.log -t 1
    ```

    文件记录的数据信息如下，用户可通过该信息了解数据对应的文件：

    ```lang-text
    Version: 0x00000001(1)
    LSN    : 0x0000000000000140(320)
    PreLSN : 0x0000000000000050(80)
    Length : 104
    Flags  : 0x0000()
    Type   : INSERT(1)
    FullName : sample.employee
    Insert : { "_id": { "$oid": "645c8054cc434dc594399897" }, "name": "Tom", "age": 20 }
    ...
    ```

    上述字段说明如下：

    | 字段名 | 描述 |
    | ------ | ---- |
    | Version | 节点切主版本号，当节点升主时递增 |
    | LSN | 当前日志对应的 LSN |
    | PreLSN | 上一条日志对应的 LSN |
    | Length | 当前日志的长度 |
    | Flags | 插入数据时执行的 flag |
    | Type | 当前日志对应的操作类型 |
    | FullName | 集合空间 |
    | Insert | 所插入集合的数据信息 |

- 解析创建索引类型的日志，并将结果输出至文件 `index.log`

    ```lang-bash
    $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/ -o index.log -t 8
    ```

    文件记录的数据信息如下：

    ```lang-text
    Version: 0x00000001(1)
    LSN    : 0x0000000000000210(528)
    PreLSN : 0x00000000000001a8(424)
    Length : 204
    Flags  : 0x0000()
    Type   : IX CREATE(8)
    CLName : sample.employee
    IXDef  : { "_id": { "$oid": "645c83aafc8d8efc581c0850" }, "UniqueID": 4294967297, "key": { "age": 1 }, "name":"ageIndex", "unique": true, "enforced": false }
    Option : { "SortBufferSize": 64, "TaskID": 1 }
    ...
    ```

    上述字段说明如下：

    | 字段名 | 描述 |
    | ------ | ---- |
    | Version | 节点切主版本号，当节点升主时递增 |
    | LSN | 当前日志对应的 LSN |
    | PreLSN | 上一条日志对应的 LSN |
    | Length | 当前日志的长度 |
    | Type | 当前日志对应的操作类型 |
    | CLName | 集合空间 |
    | IXDef | 索引的定义 |
    | Option | 索引控制参数 |

- 解析事务提交类型的日志，并将结果输出至文件 `transaction.log`

    ```lang-bash
    $ sdbdpsdump -s /opt/sequoiadb/database/data/11860/replicalog/ -o transaction.log -t 12
    ```

    文件记录的数据信息如下：

    ```lang-text
    Version: 0x00000001(1)
    LSN    : 0x0000000000000b04(2820)
    PreLSN : 0x0000000000000a88(2696)
    Length : 100
    Flags  : 0x0000()
    Type   : COMMIT(12)
    FirstLSN : 0x0000000000000a88
    Attr    : 1(Pre-Commit)
    NodeNum : 1
    Nodes   : [ (1000,1000) ]
    TransID : 0x0002001dc93091
    IDAttr  :
    TransPreLSN : 0x0000000000000a88
    ...
    ```

    上述字段说明如下：

    | 字段名 | 描述 |
    | ------ | ---- |
    | Version | 节点切主版本号，当节点升主时递增 |
    | LSN | 当前日志对应的 LSN |
    | PreLSN | 上一条日志对应的 LSN |
    | Length | 当前日志的长度 |
    | Type | 当前日志对应的操作类型 |
    | FirstLSN | 该事务第一条日志对应的 LSN |
    | Attr | 执行的操作 |
    | NodeNum | 运行该事务的节点数量 |
    | Nodes | 运行该事务的节点 ID |
    | TransID | 事务 ID |
    | TransPreLSN | 上一条事务日志对应的 LSN |