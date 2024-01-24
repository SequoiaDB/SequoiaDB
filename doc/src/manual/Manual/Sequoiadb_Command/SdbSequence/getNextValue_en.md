##NAME##

getNextValue - get the next value of current sequence

##SYNOPSIS##

**sequence.getNextValue\(\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

Get the next value of current sequence. Generally the values can be unique.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the next value as number.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Get the next value of the sequence.

```lang-javascript
> sequence.getNextValue()
1
> sequence.getNextValue()
2
> sequence.getNextValue()
3
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
