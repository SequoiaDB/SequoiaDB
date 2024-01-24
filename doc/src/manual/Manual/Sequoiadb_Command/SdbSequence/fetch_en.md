##NAME##

fetch - get multiple continuous values from current sequence

##SYNOPSIS##

**sequence.fetch\(\<num\>\)**

##CATEGORY##

SdbSequence

##DESCRIPTION##

Get multiple continuous values from current sequence. When multiple values are needed, calling this function will be faster than calling the [getNextValue()][getNextValue] repeatedly. You can specify the expected number to be fetched, but the returned number may be less than the expected.

##PARAMETERS##

num ( *number*, *required* )

The expected number to be fetched.

##RETURN VALUE##

When the function executes successfully, it will return an object, which has 3 fields. 'NextValue' is the first returned value; 'ReturnNum' is the number of returned values; 'Increment' is the interval between values.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Fetch sequence values.

```lang-javascript
> var sequence = db.createSequence( "IDSequence" )
> sequence.fetch( 10 )
{
  "NextValue": 1,
  "ReturnNum": 10,
  "Increment": 1
}
```

Print the fetched values out.

```lang-javascript
> var result = sequence.fetch( 5 ).toObj()
> var nextValue = result.NextValue
> for (var i = 1; i < result.ReturnNum; i++) {
... println( nextValue );
... nextValue += result.Increment;
... }
11
12
13
14
15
```

[^_^]:
     本文使用的所有引用及链接
[getNextValue]:manual/Manual/Sequoiadb_Command/SdbSequence/getNextValue.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
