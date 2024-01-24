[^_^]:
    SequoiaPerf-部署 Sequoiaperf Agent 服务

本文档主要介绍如何部署 SequoiaPerf Agent 服务。

##检查服务状态##

在部署 SequoiaPerf Agent 前，用户需要在每台服务器上检查 SequoiaPerf Agent 后台服务的状态。

执行如下命令，系统提示“running”表示服务正在运行：

```lang-text
# service sequoiaperf-agent status
```

如果服务未运行，可执行如下命令重新配置服务程序：

```lang-text
# service sequoiaperf-agent start
```

##部署##

用户需要通过 [sdb_perf_agent_ctl][agent_tool] 工具部署 SequoiaPerf Agent 服务。

下述示例以 SequoiaPerf Agent 安装路径为 `/opt/sequoiaperf/agent` 介绍部署步骤。

>**Note:**
>
> SequoiaPerf 服务进程全部以 SequoiaPerf 安装用户（默认为 sdbadmin）运行，需确保 SequoiaPerf 安装目录都赋予 sdnadmin 读写权限。

###启动服务###

1. 切换至 sdbadmin 用户

    ```lang-bash
    # su - sdbadmin
    ```

2. 创建 sequoiaperf_node_exporter 服务

    ```lang-bash
    $ sdb_perf_agent_ctl create sequoiaperf_node_exporter
    ```

    > **Note:**
    >
    > 用户可通过 -p 参数指定 sequoiaperf_node_exporter 服务所使用的端口。如果未指定，则使用默认端口 9100。

3. 启动服务

    ```lang-bash
    $ sdb_perf_agent_ctl start sequoiaperf_node_exporter
    ```

4. 查看服务状态

    ```lang-bash
    $ sdb_perf_agent_ctl status sequoiaperf_node_exporter
    ```

    状态为 Up 表示实例部署完成
    
    ```lang-text
    ServiceName                    PID    Port   StartTime            ServiceStatus
sequoiaperf_node_exporter      17433  9100   2021-10-27-05.55.30  Up
Total: 1; Run: 1
    ```

###将服务添加至 SequoiaPerf###

部署完成后，Agent 服务采集到的指标并不会主动发送给 SequoiaPerf 实例，需要将已启动的 Agent 服务[添加][add_agent_service]到 SequoiaPerf 实例中，由 SequoiaPerf 实例来拉取指标。




[^_^]:
    本文使用的所有引用及链接
[agent_tool]:manual/SequoiaPerf/Tools/sdb_perf_agent_ctl.md
[perf_tool]:manual/SequoiaPerf/Tools/sdb_perf_ctl.md
[add_agent_service]:manual/SequoiaPerf/Configuration/sequoiaperf_add_agent.md
