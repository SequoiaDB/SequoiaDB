
##NAME##

pull - Copy file from the remote host.

##SYNOPSIS##

***pull( \<remote_file\>, \<local_file\>, \[mode\] )***

##CATEGORY##

Ssh

##DESCRIPTION##

Copy file from the remote host.

##PARAMETERS##

| Name        | Type   | Default | Description           | Required or not |
| ----------- | ------ | ------- | --------------------- | --------------- |
| remote_file | string | ---     | remote file's path    | yes             |
| local_file  | string | ---     | local file's path     | yes             |
| mode        | int    | 0640    | set file's permission | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Connect to the host using ssh.

>**Note:**

>Suppose the local host is "192.168.20.71".

```lang-javascript
> var ssh = new Ssh( "192.168.20.72", "sdbadmin", "sdbadmin", 22 )
```

* Copy the file from the remote host.

```lang-javascript
> ssh.pull( "/opt/sequoiadb/remote_file", "/opt/sequoiadb/local_file" )
```