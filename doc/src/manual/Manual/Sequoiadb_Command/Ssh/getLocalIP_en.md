
##NAME##

getLocalIP - Get local IP adress.

##SYNOPSIS##

***getLocalIP()***

##CATEGORY##

Ssh

##DESCRIPTION##

Get local IP adress.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return local IP address.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Connect to the host using ssh.

>**Note:**

>Suppose the local host is "192.168.20.71".

```lang-javascript
> var ssh = new Ssh( "192.168.20.72", "sdbadmin", "sdbadmin", 22 )
```

* Get local IP adress.

```lang-javascript
> ssh.getLocalIP()
192.168.20.71
```