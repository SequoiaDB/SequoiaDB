
##NAME##

push - Copy file from the local host to the remote host.

##SYNOPSIS##

***push( \<local_file\>, \<dst_file\>, \[mode\] )***

##CATEGORY##

Ssh

##DESCRIPTION##

Copy file from the local host to the remote host.

##PARAMETERS##

| Name       | Type   | Default | Description             | Required or not |
| ---------- | ------ | ------- | ----------------------- | --------------- |
| local_file | string | ---     | source file's path      | yes             |
| dst_file   | string | ---     | destination file's path | yes             |
| mode       | int    | 0755    | set file's permission   | not             |

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

* Copy file from the local host to the remote host.

```lang-javascript
> ssh.push( "/opt/sequoiadb/local_file", "/opt/sequoiadb/dst_file" )
```