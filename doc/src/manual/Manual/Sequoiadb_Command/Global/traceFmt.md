##名称##

traceFmt - 解析 trace 日志

##语法##

**traceFmt(\<formatType\>, \<input\>, \<output\>)**

##类别##

Global

##描述##

该函数用于解析 trace 日志，并根据用户指定的格式化类型将结果输出至对应文件。

##参数##

- formatType（ *number，必填* ）

	格式化类型，取值如下：

    - 0：输出 trace 日志的分析文件，包含线程的执行序列（flw 文件）、函数的执行时间分析（CSV 文件）、执行时间峰值（except 文件）和 trace 记录错误信息（error 文件）
    - 1：输出解析后的 trace 日志（fmt 文件）

- input（ *string，必填* ）

    trace 日志所在路径

- output（ *string，必填* ） 

    输出文件所在路径

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`traceFmt()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -3     | SDB_PERM                  | 权限错误              | 检查输入或输出文件的路径是否存在权限问题 |
| -4     | SDB_FNE                   | 文件不存在            | 检查输入文件是否存在   |
| -6     | SDB_INVALIDARG            | 参数错误              | 检查输入的类型是否正确 |
| -189   | SDB_PD_TRACE_FILE_INVALID | 输入的文件不是 trace 日志 | 检查输入的文件是否为 [traceOff()][traceOff] 导出的 trace 日志 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。
     
##版本##

v1.0 及以上版本

##示例##

解析 trace 日志 `trace.dump`

```lang-javascript
> traceFmt(0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output")
```

>**Note:**
>
> 如果需要查看当前程序的跟踪状态，可参考 [traceStatus()][traceStatus]。

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[traceStatus]:manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md
[traceOff]:manual/Manual/Sequoiadb_Command/Sdb/traceOff.md