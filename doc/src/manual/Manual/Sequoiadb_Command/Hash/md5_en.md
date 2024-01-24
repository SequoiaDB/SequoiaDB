
##NAME##

md5 - Get the md5 value of the string.

##SYNOPSIS##

***Hash.md5( \<str\> )***

##CATEGORY##

Hash

##DESCRIPTION##

Get the md5 value of the string.

##PARAMETERS##

| Name | Type   | Default | Description | Required or not |
| ---- | ------ | ------- | ----------- | --------------- |
| str  | string | ---     | string      | yes             |

##RETURN VALUE##

On success, return string's md5 value.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Get the md5 value of the string.

```lang-javascript
> Hash.md5( "SequoiaDB" )
151a2930a718d32f64141aabda45b3b3
```