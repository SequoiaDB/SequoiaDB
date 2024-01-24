[^_^]:
    会话
    作者：杨上德
    时间：20190618
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：201090726


当数据库客户端建立一个与服务端的连接，并发送一个操作请求后，服务端通常需要保存这个操作的上下文信息，如客户端的地址信息、请求的操作类型和操作执行的进度信息等，这个上下文就是会话，是位于服务端的特定数据结构。但会话结构自身是不能执行操作的，它需要一个与之“绑定”的执行线程来对请求进行处理。

会话类型
----
会话通常和连接同时建立。在 SequoiaDB 巨杉数据库中，有很多种不同类型的会话，从其管理机制来分，可分为两大类：
+ 用户会话：由用户连接到数据库集群中的节点执行操作驱动创建。由于 SequoiaDB 的分布式特性，当用户连接到协调节点时，会在协调节点上生成一个用户会话。通常协调节点会进一步将这个请求分发到编目节点及若干个数据节点上进行执行。这个过程中会涉及到编目及数据节点的会话。

+ 系统会话：由内部的管理逻辑，根据需要自主创建，用以完成系统运行中的特定任务：连接监听、数据同步和数据持久化等。系统会话可能在节点的运行期间持续存在，或由管理算法动态创建、回收或销毁。

会话管理
----
会话通常由对应类型的会话管理器进行统一管理，会话管理器负责会话及执行线程的创建、绑定及会话的回收及销毁等。为了减少会话及线程反复创建及销毁的开销，使用会话池及线程池来对空闲的会话及线程进行回收及再利用。当需要创建一个新的会话时，先查看会话池中是否有空闲的会话，若有空闲会话，需将其从池中取出，相反则动态创建一个会话结构。同样地，线程池中如果有空闲的线程，则将其取出，相反就动态创建一个执行线程。同时将这个会话与执行线程绑定，此会话线程就可以正常处理发给该会话的请求。

会话属性
----
由于不同的业务或操作对数据库的配置要求可能存在差异，因此通常要求数据库能够灵活地根据需要进行配置调整。SequoiaDB 在多种级别上提供了控制数据库配置的能力，除了通过配置文件 API 在全局进行配置外，还支持在会话级别进行一些配置的动态设置，主要包括：
+ 读操作时的实例选择
+ 操作超时设置
+ 事务相关设置，包括隔离级别、锁等待、回滚段使用、自动提交或回滚

具体操作方式可参考 [setSessionAttr()][setSessionAttr]。

会话查询
----
在 SequoiaDB 中，用户可以通过快照来查询当前系统上的会话信息，既可以查询当前会话信息，也可以查询全局的会话信息。

* 查询全局的会话信息

   ```lang-bash
   > db.snapshot( SDB_SNAP_SESSIONS )
   ```

   输出结果如下

   ```lang-json
   {
     "NodeName": "hostname1:11810",
     "SessionID": 1,
     "TID": 8680,
     "Status": "Running",
     "Type": "LogWriter",
     "Name": "",
     "QueueSize": 0,
     "ProcessEventCount": 1,
     "RelatedID": "c0a81e442e720000000000000001",
     "Contexts": [],
     "TotalDataRead": 0,
     "TotalIndexRead": 0,
     "TotalDataWrite": 0,
     "TotalIndexWrite": 0,
     "TotalUpdate": 0,
     "TotalDelete": 0,
     "TotalInsert": 0,
     "TotalSelect": 0,
     "TotalRead": 0,
     "TotalReadTime": 0,
     "TotalWriteTime": 0,
     "ReadTimeSpent": 0,
     "WriteTimeSpent": 0,
     "ConnectTimestamp": "2013-09-27-13.28.38.927465",
     "ResetTimestamp": "2013-09-27-13.28.38.927465",
     "LastOpType": "unknow",
     "LastOpBegin": "--",
     "LastOpEnd": "--",
     "LastOpInfo": "",
     "UserCPU": "0.410000",
     "SysCPU": "0.150000"
     ···
   }
   ```

* 查询当前会话信息

   ```lang-json
   > db.snapshot( SDB_SNAP_SESSIONS_CURRENT )
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "hostname1:11820",
     "SessionID": 28,
     "TID": 9430,
     "Status": "Running",
     "Type": "Agent",
     "Name": "127.0.0.1:60309",
     "QueueSize": 0,
     "ProcessEventCount": 12,
     "RelatedID": "c0a81e442e72000000000000001c",
     "Contexts": [
       15
     ],
     "TotalDataRead": 0,
     "TotalIndexRead": 0,
     "TotalDataWrite": 0,
     "TotalIndexWrite": 0,
     "TotalUpdate": 0,
     "TotalDelete": 0,
     "TotalInsert": 0,
     "TotalSelect": 0,
     "TotalRead": 0,
     "TotalReadTime": 0,
     "TotalWriteTime": 0,
     "ReadTimeSpent": 10,
     "WriteTimeSpent": 0,
     "ConnectTimestamp": "2013-09-27-18.06.25.961090",
     "ResetTimestamp": "2013-09-27-18.06.25.961090",
     "LastOpType": "unknow",
     "LastOpBegin": "2014-08-07-14.25.23.550216",
     "LastOpEnd": "--",
     "LastOpInfo": "",
     "UserCPU": "0.910000",
     "SysCPU": "2.060000"
   }
   ```

详细操作指导及结果说明可参考[当前会话快照][snapshot_current_session]及[所有会话快照][snapshot_session]。

[^_^]:
    本文使用到的所有链接及引用

[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[snapshot_current_session]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS_CURRENT.md
[snapshot_session]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
