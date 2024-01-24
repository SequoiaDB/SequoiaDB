本文档将介绍 trace 跟踪相关的命令、开启 trace 日志收集、断点跟踪功能、输出 trace 日志和 trace 日志格式化。

##trace 相关的命令##

- **traceOn**

 开启数据库引擎跟踪功能，并且可以指定多个过滤选项；指定断点跟踪时，会在流程进入相应断点时，挂起相应线程

 语法

 ```lang-bash
 > db.traceOn(<bufferSize>, [options])
 ```

 | 参数名     | 类型           | 描述                                    | 是否必填 |
 | ---------- | -------------- | --------------------------------------- | -------- |
 | bufferSize | int32          | 缓冲区大小，单位：MB，取值：【1~1024】 | 是       |
 | options    | SdbTraceOption | 是一个 json 类型，用于指定过滤选项      | 否       |

 ```lang-bash
 SdbTraceOption({[Components: [...]], [BreakPoint: [...]], [Threads: [...]], [FunctionNames: [...]], [ThreadTypes: [...]]})
 ```

 | 参数名        | 类型        | 描述                 |
 | ------------- | :---------- | -------------------- |
 | Components | array[string] | 用于指定一组[模块][component]，默认跟踪所有模块 |
 | BreakPoint  | array[string] | 用于指定一组断点，最大断点数量为 10 个 |
 | Threads    | array[int32]    | 用于指定一组[线程][snapshot]，最大线程数量为 10 个 |
 | FunctionNames | array[string] | 用于指定一组函数名   |
 | ThreadTypes  | array[string] | 用于指定一组[线程类型][snapshot] |

- **traceResume**

 所有被断点跟踪挂起的线程恢复执行，并且将删除所有断点

 语法

 ```lang-bash
 > db.traceResume()
 ```

- **traceStatus**

 查看 trace 的状态，可以了解 trace 是否启用，当前跟踪设置的过滤选项，缓冲区的使用情况等信息

 语法

 ```lang-bash
 > db.traceStatus()
 ```

- **traceOff**

 将缓冲区中的 trace 日志导出为二进制文件

 语法

 ```lang-bash
 > db.traceOff(<dumpFile>)
 ```

 | 参数名   | 类型   | 描述                                                         | 是否必填 |
 | -------- | ------ | ------------------------------------------------------------ | -------- |
 | dumpFile | string | 导出的二进制文件的路径；如果指定文件为相对路径则存放于相应节点的 diagpath 中 | 是       |

- **traceFmt**

 完成 trace 日志的解析，按照指定的格式化类型输出相应的文本文件

 语法

 ```lang-bash
 > traceFmt(<formatType>, <input>, <output>)
 ```

 | 参数名     | 类型   | 描述                          | 是否必填 |
 | ---------- | ------ | ----------------------------- | -------- |
 | formatType | int32  | 格式化类型，取值为 0 或者 1      | 是       |
 | input      | string | traceOff 导出的二进制文件路径 | 是       |
 | output     | string | 输出的目标文件                | 是       |

##开启 trace 日志收集##

用户通过 traceOn 命令可以开启 trace 功能，通过 traceStatus 可以了解 trace 的状态，通过 traceResume 可以恢复被挂起的线程并继续跟踪。

开启 trace 日志收集需要先连接到相应节点上。

* 跟踪所有模块

   ```lang-javascript
   > db.traceOn(256)
   ```

* 按模块过滤

   ```lang-javascript
   > db.traceOn(256, SdbTraceOption({Components: ['dms']}))
   ```

* 按线程过滤

   ```lang-javascript
   > db.traceOn(256, SdbTraceOption({Threads: [4650]}))
   ```

* 按函数名过滤

   ```lang-javascript
   > db.traceOn(256, SdbTraceOption({FunctionNames: ['_dmsTempSUMgr::init']}))
   ```

* 按线程类型过滤

   ```lang-javascript
   > db.traceOn(256, SdbTraceOption({threadTypes: ['ShardAgent']}))
   ```

* 断点跟踪

   ```lang-javascript
   > db.traceOn(256, SdbTraceOption({BreakPoint: ['_dmsTempSUMgr::init']})) 
   ```

* 恢复断点执行

   ```lang-javascript
   > db.traceResume()
   ```

* 模块和线程组合过滤

   ```lang-javascript
   > db.traceOn(256, SdbTraceOption({Components: ['rtn'], Threads: [8337]}))
   ```

* 查看跟踪状态

   ```lang-javascript
   > db.traceStatus()
   ```

##trace 日志输出##

关闭跟踪，并将缓冲区中的 trace 日志以二进制格式输出到指定的文件中

```lang-javascript
> db.traceOff('tracedump.dat')
```

文件路径为绝对路径时，需要有相关目录的权限；为相对路径时，则输出到节点日志目录下面。

##trace 日志格式化##

trace 日志格式化用于将 trace 的输出日志格式化为可读的形式。

- 输出分析文件

   ```lang-bash
   > traceFmt(0, ${dbpath} + '/diaglog/tracedump.dat', ${dbpath} + '/diaglog/trace_fmt')
   ```

   按线程进行组织输出流程文件，并且针对结果作初步分析

   ```lang-text
   -rw-r----- 1 root     root                  0 Mar 25 20:35 trace_flw.error
   -rw-r----- 1 root     root                880 Mar 25 20:35 trace_flw.except
   -rw-r----- 1 root     root             129033 Mar 25 20:35 trace_flw.flw
   -rw-r----- 1 root     root               6497 Mar 25 20:35 trace_flw.funcRecord.csv
   -rw-r----- 1 root     root                 40 Mar 25 20:35 trace_flw.version
   ```
   
   其中 `trace_flw.flw` 输出为:
   
   ```lang-text
   1231:  _pmdLocalSession::_processMsg Entry(369): 2019-03-25-20.32.32.543809
   1232:  | _pmdLocalSession::_processMsg(370): 2019-03-25-20.32.32.543810
   1233:  | _pmdCoordProcessor::processMsg Entry(1756): 2019-03-25-20.32.32.543812
   1234:  | | _pmdCoordProcessor::_processCoordMsg Entry(1352): 2019-03-25-20.32.32.543813
   1235:  | | | _pmdCoordProcessor::_processCoordMsg(1354): 2019-03-25-20.32.32.543814
   1236:  | | | _netFrame::syncSend Entry(790): 2019-03-25-20.32.32.543844
   1237:  | | | | _netEventHandler::syncSend Entry(465): 2019-03-25-20.32.32.543847
   1238:  | | | | _netEventHandler::syncSend Exit(508): 2019-03-25-20.32.32.543862
   1239:  | | | _netFrame::syncSend Exit(821): 2019-03-25-20.32.32.543863
   1240:  | | | _pmdEDUCB::isInterrupted Entry(730): 2019-03-25-20.32.32.543867
   1241:  | | | | _pmdEDUCB::isInterrupted(778): 2019-03-25-20.32.32.543868
   1242:  | | | _pmdEDUCB::isInterrupted Exit(779): 2019-03-25-20.32.32.543869
   1263:  | | | _pmdEDUCB::delTransaction Entry(1066): 2019-03-25-20.32.32.545389  (1520)
   1264:  | | | _pmdEDUCB::delTransaction Exit(1072): 2019-03-25-20.32.32.545391
   1265:  | | | _pmdCoordProcessor::_processCoordMsg(1582): 2019-03-25-20.32.32.545398
   1266:  | | _pmdCoordProcessor::_processCoordMsg Exit(1617): 2019-03-25-20.32.32.545399
   1267:  | _pmdCoordProcessor::processMsg Exit(1778): 2019-03-25-20.32.32.545400
   1268:  | ossSocket::send Entry(368): 2019-03-25-20.32.32.545402
   1269:  | ossSocket::send Exit(492): 2019-03-25-20.32.32.545426
   1270:  _pmdLocalSession::_processMsg Exit(430): 2019-03-25-20.32.32.545428
   ```
   
   `trace_flw.funcRecord.csv` 输出为:
   
   ```lang-text
   name, count, avgcost, min, maxIn2OutCost, maxCurrentCost, first, second, third, fourth, fifth
   _clsGroupItem::getNodeID,1,1.0,1,1,1,"(425, 1, 1, 1)"
   _clsGroupItem::getNodeInfo,1,3.0,3,4,3,"(427, 4, 3, 1)"
   msgCheckBuffer,2,5.0,5,5,5,"(98, 5, 5, 4)","(250, 5, 5, 4)"
   msgExtractInsert,2,2.0,2,2,2,"(802, 2, 2, 1)","(1066, 2, 2, 1)"
   msgBuildQueryMsg,2,4.5,4,10,5,"(95, 10, 5, 2)","(247, 9, 4, 1)"
   msgExtractQuery,11,2.1,2,3,3,"(83, 3, 3, 2)","(87, 2, 2, 1)","(91, 2, 2, 1)","(235, 2, 2, 1)","(239, 2, 2, 1)"
   msgExtractGetMore,2,0.0,0,0,0,"(533, 0, 0, 0)","(634, 0, 0, 0)"
   msgFillGetMoreMsg,4,0.2,0,1,1,"(570, 1, 1, 1)","(176, 0, 0, 0)","(328, 0, 0, 0)","(482, 0, 0, 0)"
   _netFrame::_getHandle,1,2.0,2,2,2,"(442, 2, 2, 2)"
   _netFrame::syncSend,1,3.0,3,20,3,"(441, 20, 3, 1)"
   _netFrame::syncSend,8,3.8,3,20,5,"(125, 20, 5, 4)","(572, 15, 4, 3)","(825, 20, 4, 3)","(1089, 20, 4, 3)","(1236, 19, 4, 3)"
   _netFrame::handleMsg,17,6.1,1,20,11,"(560, 11, 11, 11)","(848, 11, 11, 11)","(148, 11, 11, 10)","(300, 11, 11, 10)","(1112, 11, 11, 10)"
   ```
   
   `trace_flw.except` 输出为:
   
   ```lang-text
   _ossEvent::wait
   sequence: 756 tid: 8193  cost: 5000110 (5000110)
   
      756:  _ossEvent::wait Entry(64): 2019-03-25-20.32.02.602858
      872:  _ossEvent::wait Exit(93): 2019-03-25-20.32.07.602968      (5000110)
   ```
   
   `trace_flw.version` 输出为：
   
   ```lang-text
   SequoiaDB version : 3.2
   Release : 39719
   ```

- 输出 dump 记录信息

   ```lang-bash
   > traceFmt(1, ${dbpath} + '/diaglog/tracedump.dat', ${dbpath} + '/diaglog/trace_fmt' )
   ```

   按照日志收集顺序，顺序格式化成可读的文本形式

   ```lang-text
   -rw-r----- 1 sdbadmin sdbadmin_group   170129 Mar 25 20:32 tracedump.out
   -rw-r----- 1 root     root             215544 Mar 25 20:34 trace_fmt.fmt
   -rw-r----- 1 root     root                 40 Mar 25 20:34 trace_fmt.version
   ```

   `trace_fmt.fmt` 输出为:
   
   ```lang-text
   1: _rtnTraceStart::doit Exit(2736): 2019-03-25-19.40.34.097959
   tid: 8369, numArgs: 1
           arg0:   0
   
   2: _pmdCoordProcessor::_processCoordMsg Normal(1582): 2019-03-25-19.40.34.097966
   tid: 8369, numArgs: 1
           arg0:   0
   
   3: _pmdCoordProcessor::_processCoordMsg Exit(1617): 2019-03-25-19.40.34.097967
   tid: 8369, numArgs: 1
           arg0:   0
   
   4: _pmdCoordProcessor::processMsg Exit(1778): 2019-03-25-19.40.34.097968
   tid: 8369, numArgs: 1
           arg0:   0
   
   5: ossSocket::send Entry(368): 2019-03-25-19.40.34.097969
   tid: 8369, numArgs: 0
   
   6: ossSocket::send Exit(492): 2019-03-25-19.40.34.097992
   tid: 8369, numArgs: 1
           arg0:   0
   
   7: _pmdLocalSession::_processMsg Exit(430): 2019-03-25-19.40.34.097994
   tid: 8369, numArgs: 1
           arg0:   0
   ```

   版本文件输出同分析文件输出。

