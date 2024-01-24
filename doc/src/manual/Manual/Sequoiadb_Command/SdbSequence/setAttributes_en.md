##NAME##

setAttributes - modify the sequence properties

##SYNOPSIS##

**sequence.setAttributes\(\<options\>\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

Modify the sequence properties such as CurrentValue, MinValue, MaxValue, etc.

##PARAMETERS##

options ( *object*, *required* )

Attributes to be modified.

- CurrentValue ( *number* )

    The current value of the sequence.

- StartValue ( *number* )

    The start value of the sequence.

- MinValue ( *number* )

    The minimum value of the sequence.

- MaxValue ( *number* )

    The maximum value of the sequence.

- Increment ( *number* )

    Each increase in the interval.

- CacheSize ( *number* )

    The number of sequence values that the catalog node caches each time.

- AcquireSize ( *number* )

    The number of sequence values that the coordinate node acquires each time.

- Cycled ( boolean )

    Whether loops are allowed when sequence values reach a maximum or minimum value.

##RETURN VALUE##

When the function executes successfully, return void.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Modify the current value of the sequence.

```lang-javascript
> sequence.setAttributes({ CurrentValue: 1000 })
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
