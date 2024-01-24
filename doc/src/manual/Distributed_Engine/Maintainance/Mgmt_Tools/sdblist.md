sdblist 是由 SequoiaDB 巨杉数据库提供，用于查看本地机器集群节点情况的工具。用户可使用该工具来查看本地机器存在哪些节点及节点的状态信息。

## 工作原理

SequoiaDB 节点启动后，会将自身的状态信息写入管道文件，供 sdblist 工具获取。SequoiaDB v2.8 及以下版本的管道文件存放于 `/tmp/sequoiadb` 目录，SequoiaDB v3.0 及以上版本的管道文件存放于 `/var/sequoiadb` 目录。当管道文件不存在时，sdblist 工具将无法显示节点的部分信息。

## 参数说明

| 参数名          | 缩写 | 描述|
| ----            | ---- | ----|
| --help          | -h   | 显示帮助信息。|
| --type          | -t   | 指定节点类型，节点类型可以为 db、om、cm 或 all，默认为 db。|
| --svcname       | -p   | 指定节点端口号，用 “,” 分隔多个端口。|
| --mode          | -m   | 指定节点模式类型，模式类型可以为 run 或 local，默认为 run。|
| --role          | -r   | 指定节点角色类型，角色类型可以为 coord、data、catalog、om 或 cm。|
| --long          | -l   | 显示详细信息。|
| --version       |      | 显示版本号。|
| --detail        |      | 显示节点配置文件信息。|
| --expand        |      | 显示节点所有项信息。|
| --long-location |      | 显示位置集详细信息。|

## 示例

   - 显示帮助信息

   ```lang-bash 
   $ sdblist -h
   Command options:
   -h [ --help ]         help
   -t [ --type ] arg     node type: db/om/cm/all, default: db
   -p [ --svcname ] arg  service name, use ',' as seperator
   -m [ --mode ] arg     mode type: run/local, default: run
   -r [ --role ] arg     role type: coord/data/catalog/om/cm
   -l [ --long ]         show long style
   --version             version
   --detail              show details
   --expand              show expanded details
   --long-location       show long style with location
   ```

   - 显示所有类型的节点

   ```lang-bash
   $ sdblist -t all
   sequoiadb(11820) (9567) D
   sdbcm(11790) (21608) 
   sequoiadb(11800) (21717) C
   sequoiadb(11810) (21760) S
   sequoiadb(11830) (21814) D
   sequoiadb(11840) (21841) D
   sdbcmd (21606)
   Total: 7
   ```

   - 显示节点详细信息

   ```lang-bash
   $ sdblist -l
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName            StartTime            DBPath
   sequoiadb  11820         data        9567      1000   1000   Y    group1               2021-02-19-15.03.20  /opt/sequoiadb/database/data/11820/
   sequoiadb  11800         catalog     21717     1      1      Y    SYSCatalogGroup      2021-02-01-14.30.55  /opt/sequoiadb/database/catalog/11800/
   sequoiadb  11810         coord       21760     2      2      Y    SYSCoord             2021-02-01-14.30.59  /opt/sequoiadb/database/coord/11810/
   sequoiadb  11830         data        21814     1001   1001   Y    group2               2021-02-01-14.31.04  /opt/sequoiadb/database/data/11830/
   sequoiadb  11840         data        21841     1002   1002   Y    group3               2021-02-01-14.31.08  /opt/sequoiadb/database/data/11840/
   Total: 5
   ```

   - 显示本地集群所有节点   

   ```lang-bash
   $ sdbstop -p 11820
   Terminating process 9567: sequoiadb(11820)
   DONE
   Total: 1; Success: 1; Failed: 0
   $ sdblist -m local
   sequoiadb(11840) (21841) D
   sequoiadb(11800) (21717) C
   sequoiadb(11820) (-) D
   sequoiadb(11830) (21814) D
   sequoiadb(11810) (21760) S
   Total: 5
   ```

   - 显示节点配置文件信息

   ```lang-bash
   $ sdblist -p 11830 --detail
   sequoiadb(11830) (21814) D
   catalogaddr       : u16-t04:11803
   dbpath            : /opt/sequoiadb/database/data/11830
   role              : data
   svcname           : 11830
   weight            : 20
   Total: 1
   ```

   - 显示节点所有项信息

   ```lang-bash
   $ sdblist -p 11830 --expand
   sequoiadb(11830) (21814) D
   confpath          : /opt/sequoiadb/bin/../conf/local/11830
   dbpath            : /opt/sequoiadb/database/data/11830/
   indexpath         : /opt/sequoiadb/database/data/11830/
   diagpath          : /opt/sequoiadb/database/data/11830/diaglog/
   auditpath         : /opt/sequoiadb/database/data/11830/diaglog/
   logpath           : /opt/sequoiadb/database/data/11830/replicalog/
   bkuppath          : /opt/sequoiadb/database/data/11830/bakfile/
   wwwpath           : /opt/sequoiadb/bin/web/
   lobpath           : /opt/sequoiadb/database/data/11830/
   lobmetapath       : /opt/sequoiadb/database/data/11830/
   maxpool           : 50
   ...
   ...
   ...
   diagnum           : 20
   auditnum          : 20
   auditmask         : SYSTEM|DDL|DCL
   ftmask            : NOSPC|DEADSYNC
   ftconfirmperiod   : 60
   ftconfirmratio    : 80
   ftlevel           : 2
   ftfusingtimeout   : 10
   syncwaittimeout   : 600
   shutdownwaittimeou: 1200
   svcname           : 11830
   svcmaxconcurrency : 0
   logwritemod       : increment
   logtimeon         : FALSE
   indexcoveron      : TRUE
   Total: 1
   ```

   - 显示节点位置集详细信息

   ```lang-bash
   $ sdblist --long-location
   Name       SvcName       Role        PID       GID    NID    PRY  GroupName             location            LocPRY  StartTime            DBPath
   sequoiadb  11820         data        9567      1000   1000   Y    group1                guangzhou           Y       2021-02-19-15.03.20  /opt/sequoiadb/database/data/11820/
   sequoiadb  11800         catalog     21717     1      1      Y    SYSCatalogGroup       beijing             Y       2021-02-01-14.30.55  /opt/sequoiadb/database/catalog/11800/
   sequoiadb  11810         coord       21760     2      2      Y    SYSCoord              -                   -       2021-02-01-14.30.59  /opt/sequoiadb/database/coord/11810/
   sequoiadb  11830         data        21814     1001   1001   N    group1                guangzhou           N       2021-02-01-14.31.04  /opt/sequoiadb/database/data/11830/
   sequoiadb  11840         data        21841     1002   1002   N    group1                -                   -       2021-02-01-14.31.08  /opt/sequoiadb/database/data/11840/
   Total: 5
   ```
