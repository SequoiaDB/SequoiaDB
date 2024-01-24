数据重分布时间定制功能

1、介绍
该功能用于定时执行数据均衡或集群缩容任务。

该功能在 <INSTALL_DIR>/tools/crontask 目录下拆分为以下两个工具：

1.1、sdbtaskdaemon
sdbtaskdaemon 是执行定时任务的守护进程。该进程以分钟为单位检查是否有任务需要执行，如果有任务则后台调用 dataRebalance.js 脚本执行该任务。

1.2、sdbtaskctl
sdbtaskctl 是管理定时任务的工具，用于创建、删除和查看定时任务。目前可指定的任务类型包括数据均衡和集群缩容。

1.2.1、数据均衡
数据均衡表示将集合上的数据均匀分布于对应的数据组中。集合对应的集合空间必须属于某个域，集合数据将在该域所包含的一个或多个数据组中实现均衡。
当集合为 range 分区时，数据均衡条件为各数据组之间的数据量小于均衡单位（默认 100MB）；当集合为 hash 分区时，各数据组之间的 partition 个数小于或等于 2。

1.2.2、集群缩容
集群缩容表示将集群中待缩容的主机数据迁移至集群的其他主机上。该工具只将待缩容主机上的数据切分至其他主机，而不会删除待缩容主机对应的数据组。

2、sdbtaskdaemon 参数说明

* --help
  返回帮助信息

* --status
  查看 deamon 进程是否存在

* --stop
  停止 deamon 进程

3、sdbtaskctl 参数说明

* create <task name>
  创建任务

* remove <task name>
  删除任务

* list
  列出任务

* --help, -h
  返回帮助信息

* -t <task type>
  指定任务类型，可填'rebalance'或者'shrink'

* -b <begin time>
  指定任务开始执行的时间，格式为'HH:mm'

* -f <frequency>
  指定任务执行的频率，可填'daily'或者'once'

* -l <collection full name>
  指定集合名
  当 -t 为'rebalance'时该参数有效

* -u <rebalance unit>
  指定均衡单位，用于判断数据是否均衡。当各数据组之间的数据量差值小于均衡单位时，该工具认为数据已均衡
  当 -t 为'rebalance'，且集合分区类型为 range 切分时该参数有效

* --hosts <host list>
  指定缩容的主机名列表，用逗号分隔，格式为'hostname1,hostname2,hostname3'
  当 -t 为'shrink'时该参数有效


4、使用

* 切换至安装路径的工具目录下
  cd /opt/sequoiadb/tools/crontask

* 运行守护进程
  setsid ./sdbtaskdaemon >> ./sdbtaskdaemon.log 2>&1

* 创建重分布任务，指定每天晚上 11 点定时做数据均衡
  ./sdbtaskctl create task1 -t rebalance -b 23:00 -f daily -l foo.bar

* 创建缩容任务，指定运行时间为晚上 11 点
  ./sdbtaskctl create task2 -t shrink -b 23:00 -f once --hosts host1,host2,host3

* 查看未执行的任务
  ./sdbtaskctl list

* 从 daemon 日志中查看执行过的任务
  vi sdbtaskdaemon.log

* 从任务日志中查看具体任务 task1 的执行情况
  vi log/task1.log


