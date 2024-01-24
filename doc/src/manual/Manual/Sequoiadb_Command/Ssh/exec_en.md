
##NAME##

exec - Execute the command.

##SYNOPSIS##

***exec( \<command\> )***

##CATEGORY##

Ssh

##DESCRIPTION##

Execute the Shell command.

##PARAMETERS##

| Name    | Type     | Default | Description   | Required or not |
| ------- | -------- | ------- | ------------- | --------------- |
| command | string   | ---     | Shell command | yes             |

##RETURN VALUE##

On success, return the resultof the command execution.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Connect to the host using ssh.

```lang-javascript
> var ssh = new Ssh( "192.168.20.71", "sdbadmin", "sdbadmin", 22 )
```

* Execute the command.

```lang-javascript
> ssh.run( "ls /opt/sequoiadb/file" )
file1
file2
file3
```
