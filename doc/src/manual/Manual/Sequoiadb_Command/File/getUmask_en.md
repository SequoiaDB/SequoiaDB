
##NAME##

getUmask - Get file mode creation mask.

##SYNOPSIS##

***File.getUmask( \[base\] )***

##CATEGORY##

File

##DESCRIPTION##

Get file mode creation mask.

##PARAMETERS##

| Name  | Type | Default | Description                                       | Required or not |
| ----- | ---- | ------- | ------------------------------------------------- | --------------- |
| base  | int  | 10      | decimal number( Octal, Decimal and Hexadecimal )  | not             |

##RETURN VALUE##

On success, return umask.
On error, exception will be thrown.

##ERRORS##

when exception happens, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Get file mode creation mask.

```lang-javascript
> File.getUmask( 8 )    // Octal
0022
> File.getUmask( 10 )   // Decimal
18
> File.getUmask( 16 )   // Hexadecimal
0x12
```