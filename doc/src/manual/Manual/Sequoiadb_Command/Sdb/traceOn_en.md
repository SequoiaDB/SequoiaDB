##NAME##

traceOn -  turn on the trace program of database engine

##SYNOPSIS##

**db.traceOn( \<bufferSize\>, [strComp], [strBreakPoint], [tids] )**

**db.traceOn( \<bufferSize\>, [SdbTraceOption] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to record each function call in the memory buffer during the execution of each command.

##PARAMETERS##

| Name | Type | Default | Description | Required or not |
| ---- | ---- | ------- | ----------- | --------------- |
| bufferSize     | number | ---         | The size of the file with trace program started. The uint is MB and the range is [1,1024] | Required |
| strComp        | string | All modules | The specify modules      | Not |
| strBreakPoint  | string | ---         | Tracing at breakpoints of specified functions ( Up to 10 breakpoints can be specified ) | Not |
| tids           | array  | All tids    | Specify one or multiple threads ( Up to 10 tids can be specified  ) | Not |
| SdbTraceOption | SdbTraceOption | ---    | Use an object to specify the monitoring parameters. For more details, refer to [SdbTraceOption][TraceOption] | Not |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `traceOn()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -187       | SDB_PD_TRACE_IS_STARTED | Trace program is already started | The trace program of database engine is currently activated and can't be restart |
| -212       | SDB_TOO_MANY_TRACE_BP | Too many trace breakpoints are specified | The number of specified breakpoints can't exceed 10 |
| -307       | SDB_OSS_UP_TO_LIMIT | Reach the maximum or minimum limit | The number of specified tids / functions / threadTypes can't exceed 10 |

When the exception happens，use [getLastError()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][general_guide].

##VERSION##

v1.0 and above

##EXAMPLES##

* Turn on the trace program of database engine. 

```lang-javascript
> db.traceOn( 256 )
```

> **Note:** 
>
> db.traceOn() only traces the nodes to which db is connected.

* Turn on the trace program of database engine, and specify the module name, breakpoints and multiple tids for tracing.

```lang-javascript
> db.traceOn( 256, "cls, dms, mth", "_dmsTempSUMgr::init", [12712, 12713, 12714] )
```

* Users can also specify monitoring parameters through SdbTraceOption.

```lang-javascript
> db.traceOn( 256, new SdbTraceOption().components( "cls", dms", "mth" ).breakPoints( "_dmsTempSUMgr::init" ).tids( [12712, 12713, 12714] ) )
```

* Check the trace status of the current program.

```lang-javascript
> db.traceStatus()
```

> **Note:**
> 
> Refer to [traceStatus()][traceStatus].

* When the traced module was blocked because of the breakpoint, users can execute the statement of [traceResume()][traceResume] to wake up the traced module.

```lang-javascript
> db.traceResume()
```

> **Note:**
>
> Refer to[traceResume()][traceResume].

* Shut down the trace program of database engine, and export the trace status to the binary file `/opt/sequoiadb/trace.dump`.

```lang-javascript
> db.traceOff("/opt/sequoiadb/trace.dump")
```

> **Note:**
>
> Refer to [traceOff()][traceOff].

* Parse binary files.

```lang-javascript
> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace.flw" )
```

> **Note:**
>
> Refer to [traceFmt()][traceFmt].

[^_^]:
    links
[TraceOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbTraceOption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[general_guide]:manual/FAQ/faq_sdb.md
[traceStatus]:manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md
[traceResume]:manual/Manual/Sequoiadb_Command/Sdb/traceResume.md
[traceOff]:manual/Manual/Sequoiadb_Command/Sdb/traceOff.md
[traceFmt]:manual/Manual/Sequoiadb_Command/Global/traceFmt.md
