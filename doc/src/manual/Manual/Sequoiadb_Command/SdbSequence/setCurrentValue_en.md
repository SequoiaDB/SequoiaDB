##NAME##

setCurrentValue - set the current value of the sequence

##SYNOPSIS##

**sequence.setCurrentValue\(\<value\>\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

This function is used to set the current value of the sequence, thereby adjusting the progress. This function does not allow to set the value back. The current value set can only be increased, not decreaseed. Descending sequences are the opposite. This feature prevents sequences from generating duplicate. To set the current value back, refer to [restart()][restart].

##PARAMETERS##

value ( *number*, *required* )

The current value to be set.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##


The common exceptions of `setCurrentValue()` function are as follows:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-361      |SDB_SEQUENCE_VALUE_USED|Sequence value has been used|To set it back, refer to [restart()][restart]|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

- Decrease the current value.

    ```lang-javascript
    > sequence.getCurrentValue()
    1000
    > sequence.setCurrentValue( 500 )
    ```

    The output error message is as follows:

    ```lang-text
    (shell):1 uncaught exception: -361
    Sequence value has been used
    ```

- Increase the current value.

    ```lang-javascript
    > sequence.getCurrentValue()
    1000
    > sequence.setCurrentValue( 2000 )
    ```

    Current value after setting:

    ```lang-javascript
    > sequence.getCurrentValue()
    2000
    ```


[^_^]:
     本文使用的所有引用及链接
[restart]:manual/Manual/Sequoiadb_Command/SdbSequence/restart.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
