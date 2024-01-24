
##NAME##

getPeerIP - Get IP address of remote host.

##SYNOPSIS##

***getPeerIP()***

##CATEGORY##

Ssh

##DESCRIPTION##

Get IP address of remote host.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return IP address of remote host.

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

* Get IP address of remote host.

```lang-javascript
> ssh.getPeerIP()
192.168.20.72
```