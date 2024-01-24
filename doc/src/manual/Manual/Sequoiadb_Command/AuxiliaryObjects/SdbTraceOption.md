用户通过该辅助类型对象可以指定 traceOn 监控参数。可指定的参数包括指定模块、断点、线程号、函数以及线程类型等。

##语法##

**SdbTraceOption() [.components( \<component1\> [,component2...] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.breakPoints( \<breakPoint1\> [,breakPoint2...] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.tids( \<tid1\> [,tid2...] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.functionNames( \<functionName1\> [,functionName2...] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.threadTypes( \<threadType1\> [,threadType2...] )]**

**SdbTraceOption() [.components( [ \<component1\>, \<component2\>, ... ] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.breakPoints( [ \<breakPoint1\>, \<breakPoint2\>, ... ] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[ .tids( [ \<tid1\>, \<tid2\>, ... ] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.functionNames( [ \<functionName1\>, \<functionName2\>, ... ] )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.threadTypes( [ \<threadType1\>,  \<threadType2\>, ... ] )]**

##方法##

- **components(\<component\>)**

 模块方法

 | 参数名       | 参数类型        | 默认值   | 描述     | 是否必填 |
 | ------------ | --------------- | -------- | -------- | -------- |
 | component    | string / array  | 所有模块 | 指定模块 | 否       |

 component 参数的可选值如下表：

 | 可选值 | 描述                      | 
 | ------ | ------------------------- | 
 | auth   | Authentication            | 
 | bps    | BufferPool Services       | 
 | cat    | Catalog Services          | 
 | cls    | Cluster Services          | 
 | dps    | Data Protection Services  | 
 | mig    | Migration Services        | 
 | msg    | Messaging Services        | 
 | net    | Network Services          | 
 | oss    | Operating System Services | 
 | pd     | Problem Determination     | 
 | rtn    | RunTime                   | 
 | sql    | SQL Parser                | 
 | tools  | Tools                     | 
 | bar    | Backup And Recovery       | 
 | client | Client                    | 
 | coord  | Coord Services            | 
 | dms    | Data Management Services  | 
 | ixm    | Index Management Services | 
 | mon    | Monitoring Services       | 
 | mth    | Methods Services          | 
 | opt    | Optimizer                 | 
 | pmd    | Process Model             | 
 | rest   | RESTful Services          | 
 | spt    | Scripting                 | 
 | util   | Utilities                 | 

- **breakPoints(\<breakPoint\>)**

 断点方法

 | 参数名       | 参数类型	      |  默认值   | 描述                    | 是否必填 |
 | ------------ | --------------- | --------- | ----------------------- | -------- |
 | breakPoint   | string / array  | ---       | 于函数处打断点进行跟踪（最多可指定 10 个断点） | 否       |

- **tids(\<tid\>)**

 线程方法

 | 参数名       | 参数类型	     |  默认值  | 描述     | 是否必填 |
 | ------------ | -------------- | -------- | -------- | -------- |
 | tid          | number / array | 所有线程 | 指定线程（最多可指定 10 个线程号） | 否       |

- **functionNames(\<functionName\>)**

 函数方法

 | 参数名       | 参数类型	     |  默认值   | 描述       | 是否必填 |
 | ------------ | -------------- | --------- | ---------- | -------- |
 | functionName | string / array | ---       | 指定函数名（最多可指定 10 个函数名） | 否       |

- **threadTypes(\<threadType\>)**

 线程类型方法

 | 参数名       | 参数类型	     | 默认值   | 描述       | 是否必填 |
 | ------------ | -------------- | -------- | ---------- | -------- |
 | threadType   | string / array | ---      | 指定线类型（最多可指定 10 个线程类型）| 否       |


 threadType 参数的可选值详见[线程类型][edu]

> **Note：**

> * SdbTraceOption 可同时连续指定多个方法。如果同时指定模块方法和函数方法或者同时指定线程类型方法和线程方法，两个方法的参数是并集关系。

> * 如果模块和函数名都不指定，则默认监控所有模块；如果指定了函数名或模块，则监控指定的函数名或模块。同理，如果线程号和线程类型都不指定，则默认监控所有线程；如果指定了线程号或线程类型，则监控指定的线程或线程类型。

##返回值##

创建对象成功时返回自身，类型为 SdbTraceOption

创建对象失败时，将抛出异常并输错误信息

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][general_guide]。

##示例##

* 开启监控程序，假设 15923 线程和 35712 线程是 SubNetAgent 类型的线程，11559 线程是 Task 类型的线程

    * 监控 oss 模块的所有函数

     ```lang-javascript
     > db = new Sdb( "localhost", 50000 )
     > var option = new SdbTraceOption().components( "oss" )
     > db.traceOn( 1000, option )
     ```

    * 监控 oss 模块的所有函数和 _pmdEDUCB::resetInfo 函数

     ```lang-javascript
     > db = new Sdb( "localhost", 50000 )
     > var option = new SdbTraceOption().components( "oss" ).functionNames( "_pmdEDUCB::resetInfo" )
     > db.traceOn( 1000, option )
     ```

    * 只监控 ossSocket::recv 函数

     ```lang-javascript
     > db = new Sdb( "localhost", 50000 )
     > var option = new SdbTraceOption().functionNames( "ossSocket::recv" )
     > db.traceOn( 1000, option )
     ```

    * 监控 SubNetAgent 类型的线程
 
     ```lang-javascript
     > db = new Sdb( "localhost", 50000 )
     > var option = new SdbTraceOption().threadTypes( "SubNetAgent" ) 
     > db.traceOn( 1000, option )
     ```

    * 只监控 11559 线程

     ```lang-javascript
     > db = new Sdb( "localhost", 50000 )
     > var option = new SdbTraceOption().tids( 11559 ) 
     > db.traceOn( 1000, option )
     ```

    * 监控 15923、35712 和 11559 线程

     ```lang-javascript
     > db = new Sdb( "localhost", 50000 )
     > var option = new SdbTraceOption().threadTypes( "SubNetAgent" ).tids( 11559 )
     > db.traceOn( 1000, option )
     ```

* SdbTraceOption 各方法的参数也可以字符串数组形式给出

    ```lang-javascript
    > db = new Sdb( "localhost", 50000 )
    > var option = new SdbTraceOption().components( [ "dms", "rtn" ] ).breakPoints( [ "_coordCMDEval::execute", "_dmsStorageUnit::insertRecord" ] ).tids( [ 15923, 35712 ] ).threadTypes( [ "RestListener", "LogWriter" ] ).functionNames( [ "_coordCMDEval::execute", "_dmsStorageUnit::insertRecord" ] )
    > db.traceOn( 1000, option )
    ```

* 对于方法中存在多个参数的情况，可以通过多次调用该方法指定参数

    ```lang-javascript
    > db = new Sdb( "localhost", 50000 )
    > var option = new SdbTraceOption().components( [ "dms", "rtn" ] ).components( [ "dps", "cls" ] ).components( "pd" )
    > db.traceOn( 1000, option )
    ```

[^_^]:
    本文使用的所有引用和链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[general_guide]:manual/FAQ/faq_sdb.md
[edu]:manual/Distributed_Engine/Architecture/Thread_Model/edu.md

