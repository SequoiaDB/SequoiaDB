[^_^]: 
     trace 分析方法
     作者：王文净
     时间：20190508
     评审意见
     王涛： 时间：
     许建辉：时间：
     市场部：时间：20190624


trace 分析主要是针对格式化后的输出文件进行分析，本文档将介绍各个输出文件如何解读，并提供 trace 分析问题的方法。

输出文件解读
----

trace 格式化输出主要有两种类型：flow 和 format 。flow 输出包括后缀名为 version、flw、except、funcRecord.csv、error 的文件；format 输出包括后缀名为 version、fmt 的文件。下面具体介始各个文件的输出及各部分的具体含义。

- 版本文件：后缀名为 version，可以通过该文件了解引擎的版本号和 Release
- 流程文件：后缀名为 flw，按线程执行流程进行组织输出，输出组成如下：

   | tid: 线程号                                   |
   | -------------------------------------- |
   | 顺序号： 函数名 Entry\|Exit [retCode=函数返回值]（行号）：时间戳 （时间间隔）      |

  > **Note：**
  >
  > 上面的 Entry 是在函数入口时才会显示，Exit 是在函数出口时才会显示，[retCode=函数返回值]也只在函数出口且有返回值时才显示，时间间隔只有在大于 1ms 的情况下才会显示，时间间隔的单位为微秒。

- 异常文件：后缀名为 except，函数调用中，两处跟踪之间最大时间间隔大于 3ms 时，该函数的跟踪记录会被输出到这个文件中

   | 函数名                                                       |
   | ------------------------------------------------------------ |
   | sequence: 顺序号  tid: 线程号 cost:  函数自身的开销（最大时间间隔） |
   | 顺序号： 函数名 Entry\|Exit [retCode=函数返回值]（行号）：时间戳 （时间间隔） |

- 汇总统计文件：后缀名为 funcRecord.csv，这个属于汇总性质的文件，其中每一行由如下几个部分组成：
  - 函数名
  - 被调用次数
  - 平均开销
  - 最小开销
  - 最大总的开销
  - 最大自身开销
  - 最大时间间隔 top 5 的跟踪记录，top 5跟踪记录的格式为: 顺序号，函数调用总的开销，函数调用自身的开销，最大时间间隔

  > **Note：**
  >
  > 函数自身的开销是把调用子函数的开销排除在外的，函数调用的总的开销是包含子函数调用的开销的，最大时间间隔一般是一个函数中最耗时的部分，所有开销的单位都微秒。

- 错误文件：后缀名为 error，错误输出文件，主要是记录存在函数出口的跟踪记录，没有匹配到函数入口的跟踪记录
- 格式化文件： 后缀名为 fmt，按照 dump 出来的二进制文件直接格式化输出的结果，其中输出各部分如下表：

   | 顺序号： 函数名 Entry           |
   | ------------------------------- |
   | tid: 线程号， numArgs: 参数数量 |
   | arg*:   对应参数具体的值        |

结合trace分析问题
----

trace 可以用来分析两类问题，常规问题和性能问题。分析常规问题时，主要关注 flw 文件，找到最开始报错的函数结合代码进行分析。

**示例**

节点正常启动的情况下，创建集合空间报 -133 错误。

1. 用户通过协调节点的诊断日志确定报错的节点，并连接到该节点，按照 trace 的输出及格式化中的方法开启 trace 跟踪

   ```lang-bash
   > var db = new Sdb("localhost:11820")
   > db.traceOn(100)
   ```

2. 创建集合空间，报 -133，重现问题

   ```lang-bash
   > db.createCS('example')
   ```

3. 导出跟踪记录为二进制文件

   ```lang-bash
   > db.traceOff("dbpath/11820.dump")
   ```

4. 格式化输出

   ```lang-bash
   > traceFmt(0, 'dbpath/11820.dump', 'dbpath/11820')\
   ```

5. 打开 `11820.flw`，找到最初报 -133 的位置

   ```lang-bash
   14502:  | | | | | sdbCatalogueCB::getAGroupRand Entry(650): 2019-04-28-17.26.49.916397
   14503:  | | | | | | sdbCatalogueCB::getAGroupRand(653): 2019-04-28-17.26.49.916397
   14504:  | | | | | sdbCatalogueCB::getAGroupRand Exit[retCode=-133](689): 2019-04-28-17.26.49.916397
   14505:  | | | | catCatalogueManager::_assignGroup Exit[retCode=-133](1057): 2019-04-28-17.26.49.916398
   14506:  | | | | pdLog Entry(431): 2019-04-28-17.26.49.916398
   ```

6. 显示 sdbCatalogueCB::getAGroupRand 最开始报 -133，结合代码是由于 grpMapId 为空导致报错：

   ```lang-cpp
   INT32 sdbCatalogueCB::getAGroupRand( UINT32 &groupID )
   {
      INT32 rc = SDB_CAT_NO_NODEGROUP_INFO ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_GETAGROUPRAND ) ;
      UINT32 mapSize = _grpIdMap.size();
      PD_TRACE1 ( SDB_CATALOGCB_GETAGROUPRAND,
                     PD_PACK_UINT ( mapSize ) ) ;
      if ( mapSize > 0 )
      {
         ...
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATALOGCB_GETAGROUPRAND, rc ) ;
      return rc;
   }  
   ```

   > **Note:**
   >
   > 向 _grpIdMap 插入元素的方法是激活组，找到激活组的命令，确认客户端的操作激活组，定位到需要调用 group.start() 。

分析性能问题，先通过 funcRecord.csv 文件找到执行次数( count )，平均开销( avgcost )或者是两者的乘积最大的跟踪记录。结合 except 找到该函数最耗时的部分定位到 flw 文件的调用栈进行分析。

1. 遇到一个操作长时间没有返回，可以通过会话快照找到执行当前操作的节点，连接到该节点并开启 trace

   ```lang-bash
   > var db = new Sdb("localhost:11850")
   > db.traceOn(100)
   ```

2. 等待一定时间，关闭 trace ，dump 跟踪记录为二进制文件

   ```lang-bash
   > db.traceOff('dbpath/11850.dump')
   ```

3. 格式化输出

   ```lang-bash
   > traceFmt(0,"dbpath/11850.dump", "dbpath/11850")
   ```

4. 打开 `funRecord.csv` 文件，按照 count 或者 avgcost 或者 count*avgcost 列进行排序

   | name                                   | count  | avgcost | min  | maxIn2OutCost | maxCurrentCost | first               | second             | third              | fourth              | fifth             |
   | -------------------------------------- | ------ | ------- | ---- | ------------- | -------------- | ------------------- | ------------------ | ------------------ | ------------------- | ----------------- |
   | dpsTransLockManager::testAcquire       | 249486 | 4.4     | 2    | 84            | 64             | (695479,79,64,60)   | (810062,75,57,52)  | (453115,52,35,31)  | (1784951,44,32,26)  | (36851,29,24,23)  |
   | _ossRWMutex::lock_r                    | 166329 | 0.4     | 0    | 11            | 11             | (260534,11,11,11)   | (420358,11,11,11)  | (923134,11,11,11)  | (328570, 10, 10,10) | (840320,10,10,10) |
   | _ossRWMutex::release_r                 | 166329 | 0.4     | 0    | 11            | 11             | (1548679,11,11,11)  | (529485, 9, 9, 9)  | (918758, 9, 9, 9)  | (1351621, 9, 9, 9)  | (721985, 8, 8, 8) |
   | dpsTransLockManager::_tryAcquireOrTest | 166325 | 3.7     | 2    | 36            | 34             | (940645, 36, 34,33) | (281898,33,32,27)  | (23643, 30, 28,26) | (450779, 26, 26,23) | (603613,27,27,23) |
   | _dmsStorageBase::_markHeaderInvalid    | 18     | 0.2     | 0    | 2             | 2              | (325029, 2, 2, 2)   | (1270007, 1, 1, 1) | (325032, 0, 0, 0)  | (325034, 0, 0, 0)   | (325036, 0, 0, 0) |

   用户从调用次数最多的函数入手，找到最大时间间隔 top 5 中的 first 里面的顺序号810062。如果从最大耗时入手，应先关联 except 文件，找到耗时最大的调用点，从调用次数入手的会直接通过顺序号去关联 flw 文件。以下是通过顺序号关联 flw 的结果：

   ```lang-text
    810062:  dpsTransLockManager::testAcquire Entry(2772): 2019-05-11-13.31.17.908202
    810063:  | dpsTransLockManager::testAcquire(2781): 2019-05-11-13.31.17.908254
    810064:  | dpsTransLockManager::testAcquire(2809): 2019-05-11-13.31.17.908258
    810065:  | dpsTransLockManager::testAcquire Entry(2772): 2019-05-11-13.31.17.908259
    810066:  | | dpsTransLockManager::testAcquire(2781): 2019-05-11-13.31.17.908261
    810067:  | | dpsTransLockManager::testAcquire(2809): 2019-05-11-13.31.17.908263
    810068:  | | dpsTransLockManager::testAcquire Entry(2772): 2019-05-11-13.31.17.908263
    810069:  | | | dpsTransLockManager::testAcquire(2781): 2019-05-11-13.31.17.908265
    810070:  | | | dpsTransLockManager::_tryAcquireOrTest Entry(1231): 2019-05-11-13.31.17.908266
    810071:  | | | | dpsTransLockManager::_tryAcquireOrTest(1264): 2019-05-11-13.31.17.908267
    810072:  | | | | _ossRWMutex::lock_r Entry(74): 2019-05-11-13.31.17.908268
    810073:  | | | | _ossRWMutex::lock_r Exit(98): 2019-05-11-13.31.17.908269
    810074:  | | | | _ossRWMutex::release_r Entry(166): 2019-05-11-13.31.17.908270
    810075:  | | | | _ossRWMutex::release_r Exit(181): 2019-05-11-13.31.17.908270
    810076:  | | | dpsTransLockManager::_tryAcquireOrTest Exit(1739): 2019-05-11-13.31.17.908270
    810077:  | | dpsTransLockManager::testAcquire Exit(2836): 2019-05-11-13.31.17.908271
    810078:  | | dpsTransLockManager::_tryAcquireOrTest Entry(1231): 2019-05-11-13.31.17.908271
    810079:  | | | dpsTransLockManager::_tryAcquireOrTest(1264): 2019-05-11-13.31.17.908273
    810080:  | | | _ossRWMutex::lock_r Entry(74): 2019-05-11-13.31.17.908274
    810081:  | | | _ossRWMutex::lock_r Exit(98): 2019-05-11-13.31.17.908275
    810082:  | | | _ossRWMutex::release_r Entry(166): 2019-05-11-13.31.17.908275
    810083:  | | | _ossRWMutex::release_r Exit(181): 2019-05-11-13.31.17.908276
    810084:  | | dpsTransLockManager::_tryAcquireOrTest Exit[retCode=-190](1739): 2019-05-11-13.31.17.908276
    810085:  | dpsTransLockManager::testAcquire Exit[retCode=-190](2836): 2019-05-11-13.31.17.908277
    810086:  dpsTransLockManager::testAcquire Exit[retCode=-190](2836): 2019-05-11-13.31.17.908277
    810087:  dpsTransLockManager::testAcquire Entry(2772): 2019-05-11-13.31.17.908279
    810088:  | dpsTransLockManager::testAcquire(2781): 2019-05-11-13.31.17.908281
    810089:  | dpsTransLockManager::testAcquire(2809): 2019-05-11-13.31.17.908283
    810090:  | dpsTransLockManager::testAcquire Entry(2772): 2019-05-11-13.31.17.908284
    810091:  | | dpsTransLockManager::testAcquire(2781): 2019-05-11-13.31.17.908286
    810092:  | | dpsTransLockManager::testAcquire(2809): 2019-05-11-13.31.17.908288
    810093:  | | dpsTransLockManager::testAcquire Entry(2772): 2019-05-11-13.31.17.908288
    810094:  | | | dpsTransLockManager::testAcquire(2781): 2019-05-11-13.31.17.908290
    810095:  | | | dpsTransLockManager::_tryAcquireOrTest Entry(1231): 2019-05-11-13.31.17.908290
    810096:  | | | | dpsTransLockManager::_tryAcquireOrTest(1264): 2019-05-11-13.31.17.908292
    810097:  | | | | _ossRWMutex::lock_r Entry(74): 2019-05-11-13.31.17.908293
    810098:  | | | | _ossRWMutex::lock_r Exit(98): 2019-05-11-13.31.17.908294
    810099:  | | | | _ossRWMutex::release_r Entry(166): 2019-05-11-13.31.17.908294
    810100:  | | | | _ossRWMutex::release_r Exit(181): 2019-05-11-13.31.17.908294
    810101:  | | | dpsTransLockManager::_tryAcquireOrTest Exit(1739): 2019-05-11-13.31.17.908295
    810102:  | | dpsTransLockManager::testAcquire Exit(2836): 2019-05-11-13.31.17.908295
   ```

   通过该线程的调用栈，可以看到这个线程一直在重复调用 dpsTransLockManager::testAcquire, 原因是报 -190 导致一直在重试解决这个问题，解决了操作长时间不返回的问题。

