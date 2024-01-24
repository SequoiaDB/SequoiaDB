[^_^]: 
    集群管理诊断日志


SequoiaDB 巨杉数据库的集群管理诊断日志记录了数据库集群被执行的所有操作。通过诊断日志，用户可以对数据库集群的运行状态进行故障分析、行为分析等操作，能有效帮助用户获取数据库集群的执行情况。

##集群管理诊断日志##

###诊断日志作用###

- 记录用户对 SequoiaDB 集群管理的行为操作
- 后台遇到的重大事件的信息
- 用于分析和定位问题

###诊断日志分类###

SequoiaDB 的集群管理诊断日志主要包括集群管理服务、集群节点和集群任务等三大模块的操作日志和运行日志，相关操作和对应的日志文件见如下表格：
  
| 类型     | 相关操作 | 日志文件 |
| -------- | -------- | -------- |
| 集群管理服务诊断日志 | 主要包括 CM 服务运行日志 | sdbcm.log<br/>sdbcm_script.log<br/>sdbcmart.log<br/>sdbcmtop.log<br/>sdbcmd.log |
| 集群节点诊断日志 | 节点启停 | sdbstart.log<br/>sdbstop.log |
| 集群任务诊断日志 | 集群异步任务操作日志 | task/1.log |

> **Note:**
>
> 集群管理日志存放路径为`数据库安装路径/conf/log/`，集群任务诊断日志存放路径为`数据库安装路径/conf/log/task/`。


###诊断日志级别###

集群管理服务诊断日志级别从高到低分为六个级别，分别以数字 0~5 表示，默认的日志级别为 3。日志级别对应关系见下表：

| 标志 | 级别    |
| ---- | ------- |
| 0    | SEVERE  |
| 1    | ERROR   |
| 2    | EVENT   |
| 3    | WARNING |
| 4    | INFO    |
| 5    | DEBUG   |

用户可以通过修改配置文件的方式，修改集群管理服务诊断日志级别，操作步骤如下：

1. 修改配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/sdbcm.conf
    ```

    将日志级别配置为 DEBUG

    ```lang-text
    DiagLevel=5
    ```

2. 重启 CM 服务

    ```lang-bash
    $ sdbcmtop
    $ sdbcmart
    ```

> **Note:**
>
> 修改日志级别操作仅对日志文件 `sdbcm.log` 和 `sdbcm_script.log` 生效。

##集群管理诊断日志解读##

集群管理诊断日志内容见下表，诊断日志解读见如下对应日志文件章节。

| 字段     | 说明                                   |
| -------- | -------------------------------------- |
| Level    | 日志级别                               |
| PID      | 进程号                                 |
| TID      | 线程号                                 |
| Function | 函数名，当前操作对应的内部函数名       |
| Line     | 函数中行号，当前日志对应的函数中的行号 |
| File     | 函数源文件                             |
| Message  | 详细信息                               |

###sdbcm.log###

`sdbcm.log` 为 CM（Cluster Manager）集群管理服务的操作和运行日志，主要包括：
- 启动和停止 CM 服务、修改 CM 服务配置以及 CM 运行日志
- 启动OM服务，以及 OM 运行日志
- 启动集群任务，以及任务相关操作日志

> **Note：**
>
> 启动 CM 服务时，`sdbcm.log` 会打印启动日志，同时会打印 CM 集群管理服务当前配置信息

启动 CM 服务，日志内容如下：

```lang-text
2019-03-14-09.27.22.444560               Level:EVENT
PID:5560                                 TID:5560
Function:pmdThreadMainEntry              Line:181
File:SequoiaDB/engine/pmd/pmdCMMain.cpp
Message:
Start cm[Version: 3.2, Release: 39774, Build: 2019-03-11-00.43.58(Enterprise-Debug)]...

2019-03-14-09.27.22.448044               Level:EVENT
PID:5560                                 TID:5560
Function:pmdThreadMainEntry              Line:230
File:SequoiaDB/engine/pmd/pmdCMMain.cpp
Message:
All configs:
defaultPort=11790
suse-054_Port=11790
RestartCount=5
RestartInterval=0
AutoStart=TRUE
DiagLevel=3
OMAddress=suse-054:11785
IsGeneral=FALSE
EnableWatch=TRUE
......
```
> **Note**
>
> 未安装 OM 服务时，OMAddress 参数值为空

[^_^]: TODO: sdbcm.conf 中配置参数在官网没有对应资料说明，可能需要独立章节说明。待确认。

###sdbcm_script.log###

`sdbcm_script.log` 为主机、集群、服务和同步任务等相关操作日志和运行日志，主要包括：
- 主机操作：扫描主机
- 集群操作：创建集群、删除集群等操作
- 服务操作：添加服务、删除服务、创建关联服务、解除关联服务等操作
- 任务操作：添加、查询、更新、删除 svc 任务等操作

日志格式：时间戳 [ 进程号 ]\[ 线程号 ][ 日志级别 ]: 详细信息

通过 SAC 扫描主机，日志内容如下：

```lang-text
2019-03-14-13:59:49.816 [31330][ 1947][  EVENT]: Begin to scan host(scanHost.js:_init)
2019-03-14-13:59:52.055 [31330][ 1947][  EVENT]: Finish scanning host(scanHost.js:_final)
```

###sdbcmart.log###

`sdbcmart.log` 主要包括 CM 集群管理命令服务的启动日志。

启动 CM 集群管理服务，日志内容如下：

```lang-text
2019-03-14-13.22.02.417051               Level:EVENT
PID:15251                                TID:15251
Function:ossStartProcess                 Line:2033
File:SequoiaDB/engine/oss/ossProc.cpp
Message:
Starting process succeed, cmd:[/opt/sequoiadb/bin/sdbcmd], pid:[15255]
```

###sdbcmtop.log###

`sdbcmtop.log` 为 CM 集群管理服务停止日志。

停止 CM 集群管理服务，日志内容如下：

```lang-text
2019-03-14-13.52.22.661281               Level:EVENT
PID:28765                                TID:28765
Function:mainEntry                       Line:379
File:SequoiaDB/engine/pmd/cm/sdbcmtop.cpp
Message:
Successful to stop sdbcm
```

###sdbcmd.log###

`sdbcmd.log` 为 CMD 集群管理命令服务的启停等操作日志。

启动 CMD，日志内容如下：

```lang-text
2019-03-14-09.27.22.435494               Level:EVENT
PID:5558                                 TID:5558
Function:mainEntry                       Line:170
File:SequoiaDB/engine/pmd/pmdCMDMNMain.cpp
Message:
Start cmd[Version: 3.2, Release: 39774, Build: 2019-03-11-00.43.58(Enterprise-Debug)]...
```

##节点诊断日志##

###sdbstart.log###

`sdbstart.log` 包括集群管理 OM 服务启动日志和集群节点启动日志。

启动 11820 节点成功，日志内容如下：

```lang-text
2019-04-13-11.25.56.318836               Level:EVENT
PID:30241                                TID:30241
Function:utilEndNodePipeDup              Line:1162
File:SequoiaDB/engine/util/utilNodeOpr.cpp
Message:
End node[11820: 30242] pipe result: 0
```

> **Note：**
>
> - Message 中[11820: 30242]为[节点端口号: 节点进程号]
> - 启动成功 pipe result 返回 0，执行失败返回对应[错误码][errorCode]

###sdbstop.log###

`sdbstop.log` 包括集群管理 OM 服务停止日志和集群节点停止日志。

停止所有节点成功，日志内容如下：

```lang-text
2019-04-13-11.25.12.343966               Level:EVENT
PID:29341                                TID:29341
Function:mainEntry                       Line:387
File:SequoiaDB/engine/pmd/sdbstop.cpp
Message:
Stop programme.
```

###task/*.log###

`task/*.log` 为集群管理异步任务操作日志，日志文件名以任务 ID 命名，任务 ID 在巨杉数据库管理员中心(SAC, SequoiaDB Administrator Center) 部署任务时显示。

集群管理异步任务包括：添加主机、删除主机、添加服务、删除服务、服务扩容、服务减容、重启服务和部署包等任务。

日志格式：

时间戳 [ 进程号 ]\[ 线程号 ][ 日志级别 ]: 详细信息

通过 SAC 扫描主机，日志内容如下：

```lang-text
2019-03-14-09:43:13.013 [ 6005][ 7513][  EVENT]: Begin to check added host info in task[1](addHostCheckInfo.js:_init)
2019-03-14-09:43:13.110 [ 6005][ 7513][  EVENT]: Finish checking added host info in task[1](addHostCheckInfo.js:_final)
```



[^_^]: 
    本文使用到的所有链接及引用
[database_conf]: manual/Manual/Database_Configuration/configuration_parameters.md
[errorCode]: manual/Manual/Sequoiadb_error_code.md
[update_conf]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md

