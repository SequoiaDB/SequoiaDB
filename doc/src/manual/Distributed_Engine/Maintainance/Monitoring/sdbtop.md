  [^_^]:
    sdbtop数据库性能工具
    作者：孙伟
    时间：20190311
    王涛：20190314
    许建辉：
    市场部：20190515
    

sdbtop 是 SequoiaDB 巨杉数据库的性能监控工具。用户可以使用 sdbtop 监控节点的操作系统资源，也可以监控集合、节点或集群的读写性能，同时支持监控会话和集合空间的信息。本文档将介绍该工具的相关信息和使用方法。

> **Note:**
>
>- sdbtop 默认每三秒刷新一次监控数据。
>- 用户应从操作系统命令行运行 sdbtop，而不是使用 shell 命令行运行。

语法规则
----

```lang-text
sdbtop [--hostname | -i arg] [--servicename | -s arg] [--usrname | -u arg] [--password | -p arg] [--confpath | -c arg]

sdbtop --ssl arg [--usrname | -u arg] [--password | -p arg] [--confpath | -c arg]

sdbtop --help | -h

sdbtop --version | -v
```

参数说明
----

| 参数 | 缩写 | 描述 |
| ---- | ---- | ---- |
| --help        | -h | 返回基本帮助和用法文本                                                             |
| --version     | -v | 返回工具版本信息                                                                   |
| --confpath    | -c | 指定 sdbtop 的配置文件路径，sdbtop 界面形态以及输出字段都依赖该文件，默认路径为 `conf/samples/sdbtop.xml`                                                                                 |
| --hostname    | -i | 指定需要监控的主机名，默认值为 localhost                                           |
| --servicename | -s | 指定监控节点的服务名或端口，默认值为 11810，如果节点为协调节点，则监控的是整个集群的信息，如果为其它节点，则只是监控其本节点信息                                                                        |
| --usrname     | -u | 指定数据库用户名，默认值为""                                                       |
| --password    | -p | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码 |
| --cipher      |    | 是否使用密文模式输入密码，默认为 false，不使用密文模式输入密码，关于密文模式的介绍可参考[密码管理][passwd] |
| --token       |    | 指定密文文件的加密令牌<br>如果创建密文文件时未指定 token，可忽略该参数                                                                       |
| --cipherfile  |    | 指定密文文件路径，默认为 `~/sequoiadb/passwd`                                      |
| --ssl         |    | 是否使用 SSL 连接，默认为 false，不使用 SSL 连接                                   |

> **Note：**
>
> - 用户通过 -i 和 -s 的参数组合可以监控本机节点、远程节点以及整个集群的信息。
> - 当数据库集群开启了用户密码鉴权，必须指定 -u 和 -p 参数


启动说明
----

* 对于 Ubuntu 等系统，需要安装 Ncurses 库，否则启动 sdbtop 将会提示“Error opening terminal: TERM”

 * 方式一：联网安装

     ```lang-bash
     $ sudo apt-get install libncurses5-dev
     ```

 * 方式二：源码安装

       1. 解压源码包

         ```lang-bash
         $ tar -xvzf ncurses-5.5.tar.gz
         ```

       2. 进入 `ncurses-5.5` 目录

         ```lang-bash
         $ ./configure
         $ sudo make && make install
         ```

* 若 Ncurses 库安装完成后仍提示“Error opening terminal: TERM”，则尝试以下解决方案：

 * 创建软连接

     ```lang-bash
     $ sudo mkdir -p /usr/share/terminfo/x
     $ cd /usr/share/terminfo/x
     $ sudo ln -s /lib/terminfo/x/xterm xterm
     ```

使用
----

sdbtop 启动后进入主窗口，主窗口中展示两种信息：界面信息导航和功能操作导航。在主窗口下通过按键 **s**、 **c**、 **t**、 **d** 和 **l** 可以进入不同的监控窗口。

```lang-text
$ sdbtop -i sdbserver1 -s svcname1 -u sdbuser1 -p sdbpassword1

refresh= 3 secs           version {version}         snapshotMode: GLOBAL
displayMode: ABSOLUTE     Main Window         snapshotModeInput: NULL
hostname: sdbserver1                          filtering Number: 0
servicename: svcname1                         sortingWay: NULL sortingField: NULL
usrName: sdbuser1                             Refresh: F5, Quit: q, Help: h
 #### ####  ####  #####  ###  ####   For help type h or ...
#     #   # #   #   #   #   # #   #  sdbtop -h: usage
 ###  #   # ####    #   #   # ####
    # #   # #   #   #   #   # #
####  ####  ####    #    ###  #

SDB Interactive Snapshot Monitor V2.0
Use these keys to ENTER:

window options(choose to enter window):
  m  : return to main window           s  : show sessions of SequoiaDB
  c  : show collection spaces          t  : show system resources
  d  : show database state             l  : show collections state
  h  : help

options(use under window above): 
  G  : show options only               g  : filter by specified group
  n  : filter by specified node        r  : set interval of refresh(second)
  A  : sort column by ascending order  D  : sort column by descending order
  C  : specify filter condition        Q  : cancel filter condition
  N  : skip specified lines ahead      W  : show all lines
 Tab : switch display Model            <- : move left
  -> : move right                      Enter: to last view, used under help window
 ESC : cancel current operation        F5  : refresh immediately
  q  : quit

Licensed Materials - Property of SequoiaDB
Copyright SequoiaDB Corp. 2013-2015 All Rights Reserved.
```

### 界面信息导航 ###

界面信息导航主要展示工具版本号、窗口刷新时间、显示模式、监控节点的机器名、监控节点的服务名或端口、数据库用户名、快照模式、排序方式以及排序字段等信息。

| 导航字段 | 描述 |
|----|----|
| refresh | 窗口刷新时间，默认每三秒刷新一次，在监控窗口下通过按键 **r** 进行更改 |
| displayMode | 显示模式，包含绝对值、差值和平均值三种模式，在监控窗口下通过按键 **Tab** 进行切换 |
| hostname | 监控节点的机器名 |
| servicename | 监控节点的服务名或端口 |
| usrName | 数据库用户名 |
| snapshotMode | 快照模式，包含全局、数据节点组和节点三种模式，在监控窗口下根据按键 **G**、 **g** 和 **n** 的输入改变 |
| snapshotModeInput | 快照模式输入，在监控窗口下通过按键 **G**、 **g** 和 **n** 输入 |
| filtering Number | 跳过 N 行显示，在监控窗口下通过按键 **N** 输入跳过的行数 |
| sortingWay | 排序方式，包含升序和降序两种方式，在监控窗口下通过按键 **A** 和 **D** 输入 |
| sortingField | 排序字段，在监控窗口下通过按键 **A** 和 **D** 输入 |
| Refresh | 立即刷新，按键 **F5** |
| Quit | 退出程序，按键 **q** |
| Help | 查看使用帮助，按键 **h** |

### 功能操作导航 ###

功能操作导航分为窗口切换按键说明和监控功能按键说明两部分。通过窗口切换按键，可以进入不同的监控窗口。监控功能按键在监控窗口下使用。

窗口切换按键说明如下：

| 窗口切换按键 | 描述 |
|----|----|
| m | 返回主窗口 |
| s | 列出数据库节点上的所有会话 |
| c | 列出数据库节点上的所有集合空间 |
| t | 列出数据库节点上的系统资源使用情况 |
| d | 列出数据库节点的数据库读写性能|
| l | 列出数据库节点的集合读写性能 |
| h | 查看使用帮助 |

> **Note:**
>
> sdbtop 每个监控窗口显示的字段长度都有固定值，超过该值的字符会被截断，但不影响功能的使用。

监控功能按键说明如下：

| 监控功能按键 | 描述 |
|----|----|
| G | 只显示功能按键的帮助信息 |
| g | 按某个数据节点组展示快照 |
| n | 按某个数据节点展示快照 |
| r | 设置窗口刷新的时间间隔，单位为秒 |
| A | 将监控信息按照某列进行升序排序 |
| D | 将监控信息按照某列进行降序排序 |
| C | 将监控信息按照某个条件进行筛选 |
| Q | 返回没有使用条件进行筛选前的监控信息 |
| N | 跳过多少行显示 |
| W | 返回没有使用行号进行过滤前的监控信息 |
| Tab | 切换数据计算的模式，支持绝对值、差值和平均值三个模式 |
| <- | 向左移动，以查看隐藏的左边列的监控信息 |
| -> | 向右移动，以查看隐藏的右边列的监控信息 |
| Enter | 返回上一次监控界面，仅在进入 help 界面后有效 |
| Esc | 取消当前的操作，比如按了升序按键 **A**，可以通过 **Esc** 键取消该操作 |
| F5 | 立即刷新 |
| q | 退出程序 |

### 示例 ###

- 进入主窗口后，按 **s** 键，列出数据库节点的所有会话信息

   ```lang-text
   refresh= 3 secs           version {version}         snapshotMode: GLOBAL
   displayMode: ABSOLUTE     Main Window         snapshotModeInput: NULL
   hostname: sdbserver1                          filtering Number: 0
   servicename: svcname1                         sortingWay: NULL sortingField: NULL
   usrName: sdbuser1                             Refresh: F5, Quit: q, Help: h
   
        SessionID TID  Type            Name                 QueueSize ProcessEventCount
        --------- ---- --------------- -------------------- --------- -----------------
     1  1         2869 Task            DATASYNC-JOB-D       0         1
     2  2         2870 Task            OptPlanClear         0         1
     3  3         2871 LogWriter       ""                   0         1
     4  4         2872 DpsRollback     ""                   0         2
     5  5         2873 Task            PAGEMAPPING-JOB-D    0         2551
     6  6         2875 DBMonitor       ""                   0         1
     8  7         2876 TCPListener     ""                   0         7
     9  8         2877 RestListener    ""                   0         1
    10  10        2879 Task            DictionaryCreator    0         1
    11  11        2880 PipeListener    ""                   0         1
    12  12        2886 Agent           127.0.0.1:59870      0         749
    13  13        2888 Unknow          ""                   0         0
    14  14        2894 Unknow          ""                   0         0
   ```

- 按 **Tab** 键，屏幕左上方的【displayMode】的值会发生切换
- 按 **r** 键，在屏幕最下方输入 2，回车，设置刷新间隔时间，屏幕左上方的【refresh】的值变为 2

   ```lang-text
   refresh= 3 secs          version {version}         snapshotMode: GLOBAL
   displayMode: AVERAGE     Main Window         snapshotModeInput: NULL
   hostname: sdbserver1                         filtering Number: 0
   servicename: svcname1                        sortingWay: NULL sortingField: NULL
   usrName: sdbuser1                            Refresh: F5, Quit: q, Help: h
   
        SessionID TID  Type            Name                 QueueSize ProcessEventCount
        --------- ---- --------------- -------------------- --------- -----------------
     1  1         2869 Task            DATASYNC-JOB-D       0         1
     2  2         2870 Task            OptPlanClear         0         1
     3  3         2871 LogWriter       ""                   0         1
     4  4         2872 DpsRollback     ""                   0         2
     5  5         2873 Task            PAGEMAPPING-JOB-D    0         2551
     6  6         2875 DBMonitor       ""                   0         1
     8  7         2876 TCPListener     ""                   0         7
     9  8         2877 RestListener    ""                   0         1
    10  10        2879 Task            DictionaryCreator    0         1
    11  11        2880 PipeListener    ""                   0         1
    12  12        2886 Agent           127.0.0.1:59870      0         749
    13  13        2888 Unknow          ""                   0         0
    14  14        2894 Unknow          ""                   0         0
   ```

- 按 **A** 键，并输入“TID”，列表结果按照 TID 进行升序排序，可以看到屏幕右上方的【sortingWay】的值变为 1（ 1 表示升序，-1 表示降序），【sortingField】的值变为 TID

   ```lang-text
   refresh= 3 secs          version {version}         snapshotMode: GLOBAL
   displayMode: AVERAGE     Main Window         snapshotModeInput: NULL
   hostname: sdbserver1                         filtering Number: 0
   servicename: svcname1                        sortingWay: 1  sortingField: TID
   usrName: sdbuser1                            Refresh: F5, Quit: q, Help: h
   
        SessionID TID  Type            Name                 QueueSize ProcessEventCount
        --------- ---- --------------- -------------------- --------- -----------------
     1  1         2869 Task            DATASYNC-JOB-D       0         1
     2  2         2870 Task            OptPlanClear         0         1
     3  3         2871 LogWriter       ""                   0         1
     4  4         2872 DpsRollback     ""                   0         2
     5  5         2873 Task            PAGEMAPPING-JOB-D    0         2551
     6  6         2875 DBMonitor       ""                   0         1
     8  7         2876 TCPListener     ""                   0         7
     9  8         2877 RestListener    ""                   0         1
    10  10        2879 Task            DictionaryCreator    0         1
    11  11        2880 PipeListener    ""                   0         1
    12  12        2886 Agent           127.0.0.1:59870      0         749
    13  13        2888 Unknow          ""                   0         0
    14  14        2894 Unknow          ""                   0         0
   ```

- 按 **N** 键，并输入 1，列表中将原来行号为 1 的记录过滤不显示
- 按 **W** 键，返回没有按行号进行过滤前的列表信息
- 按 **C** 键，并输入“TID:2869”进行筛选，则只显示 TID 值为 2869 的记录

   ```lang-text
   refresh= 3 secs          version {version}         snapshotMode: GLOBAL
   displayMode: AVERAGE     Main Window         snapshotModeInput: NULL
   hostname: sdbserver1                         filtering Number: 0
   servicename: svcname1                        sortingWay: 1  sortingField: TID
   usrName: sdbuser1                            Refresh: F5, Quit: q, Help: h
   
        SessionID TID  Type            Name                 QueueSize ProcessEventCount
        --------- ---- --------------- -------------------- --------- -----------------
     1  1         2869 Task            DATASYNC-JOB-D       0         1
   ```

- 按 **Q** 键，返回没有按照筛选条件前的列表信息
- 按 **<-** 或者 **->** 键，可以查看隐藏在左边或者右边的列


[^_^]:
     本文使用的所有引用和链接
[passwd]:manual/Distributed_Engine/Maintainance/Security/system_security.md
