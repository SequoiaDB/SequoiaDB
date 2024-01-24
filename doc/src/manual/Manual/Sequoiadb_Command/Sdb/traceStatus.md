##名称##

traceStatus - 查看当前程序跟踪的状态

##语法##

***db.traceStatus()***

##类别##

Sdb

##描述##

开启数据库引擎跟踪功能后，用户可使用该函数查看当前程序跟踪的状态。

##参数##

无

##返回值##

函数执行成功时，将通过游标（cursor）方式返回当前程序跟踪状态，返回的字段信息如下：

| 参数名        | 类型     |     描述         |
| --------------| ---------| -----------------|
| TraceStarted  | boolean  | 跟踪是否开始 <br> "true"：跟踪开始 <br> "false"：跟踪未开始                                  |
| Wrapped       | boolean  | 跟踪文件是否翻转 <br> "true"：已翻转 <br> "false"：未翻转             |
| Size          | int64    | 跟踪文件大小     |
| FreeSize      | int64    | 可用内存大小     |
| Mask          | string   | 所跟踪的模块，模块说明可参考 [SdbTraceOption][option] 的 conponent 参数     |
| BreakPoint    | string   | 所跟踪的函数断点 |
| Threads       | int32    | 线程号           |
| ThreadTypes   | string   | 线程类型，类型说明可参考 [SdbTraceOption][option] 的 threadTypes 参数         |
| FunctionNames | string   | 所跟踪的函数名   |

函数执行失败时，将抛出异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 开启数据库引擎程序跟踪的功能

    ```lang-javascript
    > db.traceOn( 100, new SdbTraceOption().components( "dms" ).functionNames( "_dmsStorageUnit::insertRecord" ).threadTypes( "RestListener" ) )
    ```

* 查看当前程序跟踪的状态：

    ```lang-javascript
    > db.traceStatus()
    {
      "TraceStarted": true,
      "Wrapped": false,
      "Size": 104857600,
      "FreeSize": 104857600,
      "PadSize": 0,
      "Mask": [
        "dms"
      ],
      "BreakPoint": [],
      "Threads": [],
      "ThreadTypes": [
        "RestListener"
      ],
      "FunctionNames": [
        "_dmsStorageUnit::insertRecord"
      ]
    }
    ```



[^_^]:
    本文使用的所有引用及链接
[option]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbTraceOption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md