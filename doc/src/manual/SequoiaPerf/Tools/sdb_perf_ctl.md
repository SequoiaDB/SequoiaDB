[^_^]:
    实例管理工具

sdb_perf_ctl 是 SequoiaPerf 的实例管理工具。用户通过 sdb_perf_ctl 可以创建和管理实例。

##参数说明##

| 参数名 | 缩写 | 描述 | 是否必填 |
| ---- | ---- | ---- | -------- |
| --coord-addrs   | -c    | 指定 SequoiaDB 集群的协调节点地址 | 是 |
| --external-ip | -e    | 指定 SequoiaPerf 页面服务器 ip | 是 |
| --sdb-user | -u    | 指定 SequoiaDB 集群的鉴权用户名 | 否 |
| --sdb-password | -w    | 指定 SequoiaDB 集群的鉴权用户密码 | 否 |
| --port | -p    | 指定 SequoiaPerf 监听的端口，默认以 14000 端口起始，监听[14000,14020]端口；如果自定义监听端口，则需要保证延后的 20 个端口都可供 SequoiaPerf 实例使用 | 否 |
| --instance-path | -d    | 指定 SequoiaPerf 实例的数据目录，默认为 `SequoiaPerf 安装目录/instances` | 否 |
| --log-path | -l    | 指定服务日志的存储路径  | 否 |
| --log-level |  | 指定服务的日志级别，取值包括 debug、info、warn、error  | 否 |
| --tsdb-path |  | 指定 sequoiaperf_prometheus 服务的数据存储路径，默认为 `./data`  | 否 |
| --tsdb-retention-time |  | 指定 sequoiaperf_prometheus 服务的数据保留时间，默认为 15d，可配置的单位包括 ms、s、m、h、d、w、y  | 否 |
| --tsdb-retention-size |  | 指定 sequoiaperf_prometheus 服务的数据保留大小，默认为 1GB，可配置的单位包括 KB、MB、GB  | 否 |
| --language |  | 指定 SequoiaPerf 页面语言，取值包括 cn 和 en，默认为 cn  | 否 |
| --print |  | 打印日志信息  | 否 |
| --verbose | -V    | 显示详细的执行信息 | 否 |
| --help | -h    | 显示帮助信息 | 否 |
| --version | -v    | 显示版本信息 | 否 |

##使用说明##

运行 sdb_perf_ctl 的用户必须与安装 SequoiaPerf 时指定的用户一致。

###管理实例###

- 创建实例

    sdb_perf_ctl addinst \<INSTNAME\> <-c COORD_ADDR> <-e EXTERNAL_IP> [-p PORT] ] [-d DATA_DIR] [-u SDB_USER] [-w SDB_PASSWORD] [--language] [--print]

    创建名为“SequoiaPerf1”的实例，并指定协调节点地址、页面服务器 ip 等参数

    ```lang-bash
    $ sdb_perf_ctl addinst SequoiaPerf1 -c 127.0.0.1:11810 -e 192.168.31.20 -d /tmp -u sdbtest -w sdbpasswd --language cn --print
    ```

    >**Note:**
    >
    > -w 支持指定明文密码或 MD5 密码。

- 启动实例

    sdb_perf_ctl start \<INSTNAME\>

    启动实例 SequoiaPerf1

    ```lang-bash
    $ sdb_perf_ctl start SequoiaPerf1
    ```

- 重启实例
  
    sdb_perf_ctl restart \<INSTNAME\>

    重启实例 SequoiaPerf1

    ```lang-bash
    $ sdb_perf_ctl restart SequoiaPerf1
    ```

- 停止实例

    sdb_perf_ctl stop \<INSTNAME\>

    停止实例 SequoiaPerf1

    ```lang-bash
    $ sdb_perf_ctl stop SequoiaPerf1
    ```

- 删除实例
  
    sdb_perf_ctl delinst \<INSTNAME\>

    删除实例 SequoiaPerf1

    ```lang-bash
    $ sdb_perf_ctl delinst SequoiaPerf1
    ```

    >**Note:**
    >
    > 删除 SequoiaPerf 实例后，该实例的所有日志、数据和配置将永久删除。

- 启动实例的[服务][Readme]

    sdb_perf_ctl start \<INSTNAME\> [SERVICE]

    重启实例 SequoiaPerf1 的服务 sequoiaperf_gateway

    ```lang-bash
    $ sdb_perf_ctl start SequoiaPerf1 sequoiaperf_gateway
    ```

- 重启实例的服务

    sdb_perf_ctl restart \<INSTNAME\> [SERVICE]

    重启实例 SequoiaPerf1 的服务 sequoiaperf_gateway

    ```lang-bash
    $ sdb_perf_ctl restart SequoiaPerf1 sequoiaperf_gateway
    ```

- 停止实例的服务

    sdb_perf_ctl stop \<INSTNAME\> [SERVICE]

    停止实例 SequoiaPerf1 的服务 sequoiaperf_gateway

    ```lang-bash
    $ sdb_perf_ctl stop SequoiaPerf1 sequoiaperf_gateway
    ```

- 检查实例状态

    sdb_perf_ctl status [INSTNAME]

    检查所有实例的状态

    ```lang-bash
    $ sdb_perf_ctl status
    ```

- 查看所有添加的实例

   ```lang-bash
   $ sdb_perf_ctl listinst
   ```

- 启动所有实例

   ```lang-bash
   $ sdb_perf_ctl startall
   ```

- 停止所有实例

   ```lang-bash
   $ sdb_perf_ctl stopall
   ```


###修改实例配置###

用户可通过 sdb_perf_ctl 修改实例或指定服务的配置，下述将介绍可修改的配置及修改方法。

```lang-text
sdb_perf_ctl chconf <INSTNAME> <--language=LANGUAGE>
                    <INSTNAME> <SERVICE> [-l LOGPATH] [--log-level LOGLEVEL]
                    <INSTNAME> sequoiaperf_exporter [-u SDBUser] [-w SDBPassword] [-c CoordAddr]
                    <INSTNAME> sequoiaperf_prometheus [--tsdb-path PATH] [--tsdb-retention-time TIME] [--tsdb-retention-size SIZE]
```


**修改页面语言**

修改 SequoiaPerf 的页面语言为中文

```lang-bash
$ sdb_perf_ctl chconf SequoiaPerf1 --language=cn
```

**修改 sequoiaperf_exporter 配置**

修改 SequoiaPerf1 实例 sequoiaperf_exporter 服务的 SequoiaDB 集群连接信息

```lang-bash
$ sdb_perf_ctl chconf SequoiaPerf1 sequoiaperf_exporter -c 192.168.31.38:11810 -u sdbadmin -w sdbadmin
```

**修改服务的日志配置**

1. 修改 SequoiaPerf1 实例 sequoiaperf_gateway 服务的日志路径和日志级别

    ```lang-bash
    $ sdb_perf_ctl chconf SequoiaPerf1 sequoiaperf_gateway -l /tmp/logs --log-level=warn
    ```

2. 重启实例的服务，使配置生效

    ```lang-bash
    $ sdb_perf_ctl restart SequoiaPerf1 sequoiaperf_gateway
    ```

**修改 sequoiaperf_prometheus 配置**

1. 修改 SequoiaPerf1 实例 sequoiaperf_prometheus 服务的数据存储配置

    ```lang-bash
    $ sdb_perf_ctl chconf SequoiaPerf1 sequoiaperf_prometheus --tsdb-path=/tmp/tsdb --tsdb-retention-time=1d --tsdb-retention-size=1GB 
    ```

2. 重启实例的服务，使配置生效

    ```lang-bash
    $ sdb_perf_ctl restart SequoiaPerf1 sequoiaperf_prometheus
    ```





[^_^]:
    本文使用的所有引用及链接
[Readme]:manual/SequoiaPerf/Readme.md
