##NAME##

getCurrentValue - get the current value of the sequence

##SYNOPSIS##

**sequence.getCurrentValue\(\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

Get the current value of the sequence to know the progress.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the current value as number.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `getCurrentValue()`:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-362      |SDB_SEQUENCE_NEVER_USED|Sequence has never been used|Check whether the sequence has ever been used|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

* Get the current value of a new sequence, and failed.

    ```lang-javascript
    > var sequence = db.createSequence("IDSequence")
    > sequence.getCurrentValue()
    (shell):1 uncaught exception: -362
    Sequence has never been used
    ```

* Get it again after using the sequence, and succeed.

    ```lang-javascript
    > sequence.getNextValue()
    1
    > sequence.getCurrentValue()
    1
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
