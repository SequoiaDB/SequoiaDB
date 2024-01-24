资源管理节点（sdbcm）是一个守护进程，以服务的方式常驻系统后台。SequoiaDB 巨杉数据库的所有集群管理操作都必须有 sdbcm 的参与，目前每一台物理机器上只能启动一个 sdbcm 进程，负责执行远程的集群管理命令和监控本地的 SequoiaDB 巨杉数据库。sdbcm 主要有两大功能：

- 远程启动、关闭、创建和修改节点：通过 SequoiaDB 客户端或者驱动连接数据库时，可以执行启动、关闭、创建和修改节点的操作，该操作向指定节点物理机器上的 sdbcm 发送远程命令，并得到 sdbcm 的执行结果。

- 本地监控：sdbcm 通过维护节点列表，实现对集群的节点监控。节点列表中保存 sdbcm 所管理节点的信息，如服务名、启动信息、运行状态等。如果某个节点非正常终止（节点被强制终止，引擎异常退出等），sdbcm 会尝试重启该节点。

sdbcm 操作
----

- 配置文件

  在数据库安装目录的 `conf` 子目录下，有一个 `sdbcm.conf` 的配置文件，该文件给出了启动 sdbcm 时的配置信息，如下所示：


  | 参数                  | 描述        | 示例                        |
  |-----------------------|-------------|-----------------------------|
  | defaultPort           | sdbcm 的默认监听端口 | defaultPort=11790  |
  | <hostname>_Port | 物理主机 hostname 上 sdbcm 的监听端口，若在该配置文件中找不到对应主机的参数，sdbcm 会以 defaultPort 启动；若 defaultPort 不存在，则 sdbcm 以默认端口 11790 启动                                    | &lt;hostname&gt;_Port=11790 |
  | RestartCount          | 重启次数，即定义 sdbcm 对节点的最大重启次数；该参数不存在时默认值为 -1，即不断重启                  | RestartCount=5              |
  | RestartInterval       | 重启间隔，即定义 sdbcm 的最大重启间隔，默认值为 0，即不考虑重启间隔，单位为分钟；该参数与 RestartCount 结合定义了重启间隔内 sdbcm 对节点的最大重启次数，超出时则不再重启     | RestartInterval=0           |
  | DiagLevel             | 指定诊断日志打印级别，SequoiaDB 中诊断日志从 0~5 分别代表：SEVERE、ERROR、EVENT、WARNING、INFO、DEBUG；如果不指定，则默认为 WARNING | DiagLevel=3 |
  | AutoStart             | sdbcm 启动时是否自动拉起其他节点进程，如果不指定则默认为 false，即不自动拉起其他节点进程    | AutoStart=TRUE |
  | EnableWatch           | 是否监控节点，即是否重启异常节点，如果不指定则默认为TRUE，即监控节点 | EnableWatch=TRUE |

- 启动 sdbcm

  运行 `sdbcmart` 命令可以启动 sdbcm

  ```lang-bash
  $ sdbcmart
  Success: sdbcmd is already started (1078)
  ```

  检查 sdbcm 进程

  ```lang-bash
  $ ps -elf | grep sdbcm
  0 S sdbadmin  1078     1  0  80   0 -  6595 hrtime 16:49 pts/0    00:00:00 sdbcmd
  0 S sdbadmin  1080     1  0  80   0 - 124902 hrtime 16:49 pts/0   00:00:00 sdbcm(11790)
  0 S sdbadmin  1107 28749  0  80   0 -  3669 pipe_w 16:59 pts/0    00:00:00 grep --color=auto sdbcm
  ```

- 关闭 sdbcm

  运行 `sdbcmtop` 命令可以关闭 sdbcm

  ```lang-bash
  $ sdbcmtop
  Terminating process 1080: sdbcm(11790)
  DONE
  Successful to stop sdbcm
  ```


