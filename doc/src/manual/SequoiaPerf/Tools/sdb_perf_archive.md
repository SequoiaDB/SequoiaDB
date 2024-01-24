[^_^]:
    归档管理工具

sdb_perf_archive 是 SequoiaPerf 的归档管理工具，用户通过该工具可以创建、删除和管理实例的归档。归档功能可以将历史数据保留，便于分析和诊断性能问题。

恢复归档时，SequoiaPerf 将根据归档文件创建新实例，并在新实例上进行数据恢复。恢复后的数据为只读模式，仅供查询。

##参数说明##

| 参数名 | 缩写 | 描述 | 是否必填 |
| ----   | ---- | ---- | -------- |
| --file | -f    | 指定 SequoiaPerf 归档文件名 | 是 |
| --external-ip | -e    | 指定 Sequoiaperf 恢复后页面服务器 ip  | 是 |
| --port | -p    | 指定 Sequoiaperf 恢复后实例的端口  | 是 |
| --instance-path | -i    | 指定 Sequoiaperf 恢复后实例的数据目录  | 是 |
| --data-path | -d    | 指定 SequoiaPerf 归档存放的目录，默认为 `SequoiaPerf 安装目录/archives` | 否 |
| --print | | 打印日志信息  | 否 |
| --version | -v | 显示版本信息  | 否 |

##使用说明##

运行 sdb_perf_archive 的用户必须与安装 SequoiaPerf 时指定的用户一致。


- 创建归档

    sdb_perf_archive create \<INSTNAME\> [-d DIRECTORY] [--print]

    在目录 `/opt/sequoiaperf/mybackup/` 下创建归档，同时打印日志信息

    ```lang-bash
    $ sdb_perf_archive create perf1 -d /opt/sequoiaperf/mybackup/ --print
    ```

- 查看归档

    sdb_perf_archive list [INSTNAME]  [--print]

    查看所有归档

    ```lang-bash
    $ sdb_perf_archive list
    ```

- 恢复归档

    sdb_perf_archive restore \<INSTNAME\> <-f FILE> <-e EXTERNAL_IP> [-p PORT] [-i INSTANCE_PATH] [--print]

    在实例 perf2 中恢复名为“2021_10_11_20_16_20_441_perf1.tar.gz”的归档，并指定 perf2 的页面服务器 ip 为 192.168.31.38

    ```lang-bash
    $ sdb_perf_archive restore perf2 -f ./archives/perf1/2021_10_11_20_16_20_441_perf1.tar.gz -e 192.168.31.38
    ```

    >**Note:**
    >
    > 归档恢复后，用户可以在 SequoiaPerf 页面，以指定时间区间的方式查看指定的历史数据。

- 删除归档
  
    sdb_perf_archive delete <-f FILE>  [--print]

    删除归档 `./archives/perf1/2021_10_11_20_16_20_441_perf1.tar.gz`

    ```lang-bash
    $ sdb_perf_archive delete -f ./archives/perf1/2021_10_11_20_16_20_441_perf1.tar.gz
    ```
