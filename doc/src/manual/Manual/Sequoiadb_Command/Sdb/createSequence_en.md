##NAME##

createSequence - create a sequence in current database

##SYNOPSIS##

**db.createSequence\(\<name\>, \[options\]\)**

##CATEGORY##

Sdb

##DESCRIPTION##

Create a sequence in current database.

##PARAMETERS##

+ name ( *string*, *required* )

    Sequence name. It cannot begin with 'SYS' or '$'.

+ options ( *object*, *optional* )

    The sequence attributes.

    1. StartValue ( *number* )

        The start value of the sequence. The default value is 1.

    2. MinValue ( *number* )

        The minimum value of the sequence. The default value is 1.

    3. MaxValue ( *number* )

        The maximum value of the sequence. The default value is 2^63 -1.

    4. Increment ( *number* )

        Each increase in the interval. The default value is 1.  It can be positive or negative integers, but cannot be 0.

    5. CacheSize ( *number* )

        The number of sequence values that the catalog node caches each time. The default value is 1000.  It must be greater than zero.

    6. AcquireSize ( *number* )

        The number of sequence values that the coordinate node acquires each time. The default value is 1000.  It must be less than or equal to CacheSize.

    7. Cycled ( *boolean* )

        Whether loops are allowed when sequence values reach a maximum or minimum value. Its default value is false.

##RETURN VALUE##

When the function executes successfully, it will return the created sequence object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `createSequence()`:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-323      |SDB_SEQUENCE_EXIST|Sequence already exists|Check for sequence existence|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Create a sequence with options, and operate it.

```lang-javascript
> var sequence = db.createSequence( "IDSequence", { Cycled: true } )
> sequence.help()
> sequence.getNextValue()
```


[^_^]:    
    links
[getLastErrMsg]:manual/reference/Sequoiadb_command/Global/getLastErrMsg.md
[getLastError]:manual/reference/Sequoiadb_command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
