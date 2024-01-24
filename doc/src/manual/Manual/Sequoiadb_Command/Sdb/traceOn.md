##名称##

traceOn - 开启数据库引擎跟踪功能

##语法##

**db.traceOn( \<bufferSize\>, [strComp], [strBreakPoint], [tids] )**

**db.traceOn( \<bufferSize\>, [SdbTraceOption] )**

##类别##

Sdb

##描述##

该函数用于将每个命令执行过程中的每个函数调用都记录在内存缓冲区中。

##参数##

| 参数名   | 类型 | 默认值  | 描述 | 是否必填 |
| -------- | -------- | ------- | ---- | -------- |
| bufferSize     | number  | ---      | 开启追踪的文件大小，单位为兆字节，取值范围为[1,1024] | 是 |
| strComp        | string    | 所有模块 | 指定模块，可选模块请参考 [component][component]    | 否 |
| strBreakPoint  | string    | ---      | 在指定函数处打断点进行跟踪（最多可指定 10 个断点） | 否 |
| tids           | array     | 所有线程 | 指定单个或多个线程 tid（最多可指定 10 个线程号） | 否 |
| SdbTraceOption | SdbTraceOption | ---      | 使用一个对象来指定监控参数，使用方法可参考 [SdbTraceOption][TraceOption] | 否 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`traceOn()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决方法 |
| ------ | -------- | -------------- | -------- |
| -187   | SDB_PD_TRACE_IS_STARTED | 跟踪已经启动 | 当前已启动数据库引擎跟踪功能，不能重复启动 |
| -212   | SDB_TOO_MANY_TRACE_BP | 跟踪断点数量过多 | 断点指定的数量不能超过 10 个 |
| -307   | SDB_OSS_UP_TO_LIMIT | 达到最大或最小限制 | 线程号、函数名或者线程类型指定的数量不能超过 10 个 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][general_guide]。

##版本##

v1.0 及以上版本

##示例##

* 开启数据库引擎程序跟踪的功能

    ```lang-javascript
    > db.traceOn( 256 )
    ```

   > **Note:**
   >
   > db.traceOn() 只对 db 所连接的节点进行跟踪。

* 开启数据库引擎程序跟踪功能，并指定模块名称、断点和多个 tid 进行跟踪

   ```lang-javascript
   > db.traceOn( 256, "cls, dms, mth", "_dmsTempSUMgr::init", [12712, 12713, 12714] )
   ```

   也可以通过 SdbTraceOption 指定监控参数
  
   ```lang-javascript
   > db.traceOn( 256, new SdbTraceOption().components( "cls", dms", "mth" ).breakPoints( "_dmsTempSUMgr::init" ).tids( [12712, 12713, 12714] ) )
   ```

* 查看当前程序跟踪的状态

    ```lang-javascript
    > db.traceStatus()
    ```

   > **Note:**
   > 
   > 可参考 [traceStatus()][traceStatus]

* 当被跟踪的模块遇到断点被阻塞，可以执行如下语句唤醒被跟踪的模块：

    ```lang-javascript
    > db.traceResume()
    ```

   > **Note:**
   >
   > 可参考 [traceResume()][traceResume]

* 关闭数据库引擎跟踪，并将跟踪情况导出二进制文件 `/opt/sequoiadb/trace.dump`

    ```lang-javascript
    > db.traceOff("/opt/sequoiadb/trace.dump")
    ```

   > **Note:**
   >
   > 可参考 [traceOff()][traceOff]

* 解析二进制文件

    ```lang-javascript
    > traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace.flw" )
    ```

   > **Note:**
   >
   > 可参考 [traceFmt()][traceFmt] 

[^_^]:
    本文使用的所有引用和链接
[TraceOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbTraceOption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[general_guide]:manual/FAQ/faq_sdb.md
[traceStatus]:manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md
[traceResume]:manual/Manual/Sequoiadb_Command/Sdb/traceResume.md
[traceOff]:manual/Manual/Sequoiadb_Command/Sdb/traceOff.md
[traceFmt]:manual/Manual/Sequoiadb_Command/Global/traceFmt.md
[component]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbTraceOption.md#方法
