##NAME##

traceFmt - parse the trace log

##SYNOPSIS##

**traceFmt(\<formatType\>, \<input\>, \<output\>)**

##CATEGORY##

Global

##DESCRIPTION##

This function is used to parse the trace log and output the result to the corresponding file according to the format type specified by the user.

##PARAMETERS##

- formatType ( *number, required* )

	Formate type, the values are as follows:

    - 0：Output trace log analysis file, including thread execution sequence(flw file), function execution time analysis(CSV file), execution time peak(except file) and trace error information(error file).
    - 1：Output the parsed trace log(fmt file).

- input ( *string, required* )

    The path where the trace log is located.

- output ( *string, required* ) 

    The path where the output file is located.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `traceFmt()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | -------- | -------------- | -------- |
| -3     | SDB_PERM                  | Permissions error              | Check whether the path of the input or output file has permission. |
| -4     | SDB_FNE                   | File dose not exist            | Check whether the input file exists.   |
| -6     | SDB_INVALIDARG            | Parameter error              | Check whether the input type is correct. |
| -189   | SDB_PD_TRACE_FILE_INVALID | The input file is not a trace log | Check whether the input file is [traceOff()][traceOff] exported trace log. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].
     
##VERSION##

v1.0 and above

##EXAMPLES##

Parsing trace logs `trace.dump`.

```lang-javascript
> traceFmt(0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output")
```

>**Note:**
>
> If users want to view the trace status of the current program, refer to [traceStatus()][traceStatus].

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[traceStatus]:manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md
[traceOff]:manual/Manual/Sequoiadb_Command/Sdb/traceOff.md