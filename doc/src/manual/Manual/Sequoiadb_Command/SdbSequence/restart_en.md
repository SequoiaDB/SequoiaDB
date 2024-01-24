##NAME##

restart - restart the sequence from the given value

##SYNOPSIS##

**sequence.restart\(\<value\>\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

This function is used to restart the sequence from the given value. The given initial value can be any value in the range from the minimun to the maximun of the sequence.

##PARAMETERS##

value ( *number*, *required* )

The new start value.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Recount the sequence with the current value of 10 to 1.

```lang-javascript
> sequence.getNextValue()
10
> sequence.restart()
> sequence.getNextValue()
1
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
