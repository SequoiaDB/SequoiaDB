[^_^]:
    进程信号量操作Readme

SequoiaDB 巨杉数据库进程信号量操作说明。
> **Note:**  
> 仅在 Linux 平台生效

##信号量##

| 信号量       | 描述                                                                                    |
| ------------ | --------------------------------------------------------------------------------------- |
| 23           | 生成线程堆栈和内存状态、内存跟踪文件                                                    |
| 35           | 测试信号，无实际操作                                                                    |
| 37           | 挂起和恢复进程，需要与配置参数 `enablesleep` 才生效                                     |
| 39           | 生成内存状态、内存跟踪文件，以及系统内存状态文件                                        |
| 41           | 回收系统空闲内存(包括 Thread, MemPool 和 System )                                       |

通过 shell 操作 `kill -\<signal\> \<pid\>` 给进程发送上述信号量，生成文件存放在节点日志目录(diagpath)下，文件名格式如下：

- 线程堆栈文件： \<pid\>.\<tid\>.trap

- 内存跟踪信息文件（需要开启内存调试参数 `memdebug` 和 `memdebugmask` 才会产生相应文件）：
   - \<pid\>.memossdump
   - \<pid\>.mempooldump
   - \<pid\>.memtcdump

- 内存状态文件：
   - \<pid\>.mempoolstat
   - \<pid\>.memtcstat

- 系统内存状态文件：
   - \<pid\>.memsysstat
   - \<pid\>_\<Format Time\>.memsysinfo_xml


[^_^]:
     本文使用的引用和键接：
