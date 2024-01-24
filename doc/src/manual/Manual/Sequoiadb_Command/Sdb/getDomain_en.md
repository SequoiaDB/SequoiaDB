##NAME##

getDomain - get the specified domain

##SYNOPSIS##

**db.getDomain(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the specified domain.

##PARAMETERS##

|Name      |type        |Description  |Required or not |
|--------- |----------- |------------ |----------|
| name | string | Domain name | required |

> **Note:**
>
> Does not support obtaining system domain "SYSDOMAIN".

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbDomain. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Get a previously created domain.

```lang-javascript
> var domain = db.getDomain('mydomain')
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md