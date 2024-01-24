[^_^]:
    Agent 管理工具

sdb_perf_agent_ctl 是 SequoiaPerf 的 Agent 服务管理工具。用户通过 sdb_perf_agent_ctl 可以创建和管理 Agent 服务。

##参数说明##

| 参数名 | 缩写 | 描述 | 是否必填 |
| ---- | ---- | ---- | -------- |
| --port | -p    | 指定 Agent 服务监听的端口 | 否 |
| --log-dir | -l    | 指定服务日志的存储路径  | 否 |
| --log-level |  | 指定服务的日志级别，取值包括 debug、info、warn、error  | 否 |
| --print |  | 打印日志信息  | 否 |
| --verbose | -V    | 显示详细的执行信息 | 否 |
| --help | -h    | 显示帮助信息 | 否 |
| --version | -v    | 显示版本信息 | 否 |

##使用说明##

运行 sdb_perf_agent_ctl 的用户必须与安装 SequoiaPerf Agent 时指定的用户一致。

- 创建[服务][perf_readme]

    sdb_perf_agent_ctl create \<SERVICE\> [-p PORT] ] [-l LOG_DIR] [--log-level LOG_LEVEL] [--print]

    创建 sequoiaperf_node_exporter 服务，并指定端口号为 9200

    ```lang-bash
    $ sdb_perf_agent_ctl create sequoiaperf_node_exporter -p 9200
    ```

    > **Note:**
    >
    > 如果不指定端口号，则默认端口号为 9100。

- 启动服务

    sdb_perf_agent_ctl start \<SERVICE\>

    启动 sequoiaperf_node_exporter 服务

    ```lang-bash
    $ sdb_perf_agent_ctl start sequoiaperf_node_exporter
    ```

- 重启服务
  
    sdb_perf_agent_ctl restart \<SERVICE\>

    重启 sequoiaperf_node_exporter 服务

    ```lang-bash
    $ sdb_perf_agent_ctl restart sequoiaperf_node_exporter
    ```

- 停止服务

    sdb_perf_agent_ctl stop \<SERVICE\>

    停止 sequoiaperf_node_exporter 服务

    ```lang-bash
    $ sdb_perf_agent_ctl stop sequoiaperf_node_exporter
    ```

- 删除服务
  
    sdb_perf_agent_ctl delete \<SERVICE\>

    删除 sequoiaperf_node_exporter 服务

    ```lang-bash
    $ sdb_perf_agent_ctl delete sequoiaperf_node_exporter
    ```

    >**Note:**
    >
    > 删除服务后，该服务的所有日志、数据和配置将永久删除。


- 检查服务状态

    sdb_perf_agent_ctl status \<SERVICE\>

    检查 sequoiaperf_node_exporter 服务的状态

    ```lang-bash
    $ sdb_perf_agent_ctl status sequoiaperf_node_exporter
    ```

- 检查所有服务状态

    ```lang-bash
    $ sdb_perf_agent_ctl statusall
    ```

- 查看所有已创建的服务

   ```lang-bash
   $ sdb_perf_agent_ctl list
   ```

- 启动所有服务

   ```lang-bash
   $ sdb_perf_agent_ctl startall
   ```

- 停止所有服务

   ```lang-bash
   $ sdb_perf_agent_ctl stopall
   ```

- 重启所有服务

   ```lang-bash
   $ sdb_perf_agent_ctl restartall
   ```





[^_^]:
    本文使用的所有引用及链接
[perf_readme]:manual/SequoiaPerf/Readme.md
