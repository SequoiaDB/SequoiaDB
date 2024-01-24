
##NAME##

Ssh - Connect to the host using ssh.

##SYNOPSIS##

***var ssh = new Ssh( \<hostname\>, \<user\>, \[password\], \[port\] )***

##CATEGORY##

Ssh

##DESCRIPTION##

Connect to the host using ssh.

##PARAMETERS##

| Name     | Type     | Default | Description         | Required or not |
| -------- | -------- | ------- | ------------------- | --------------- |
| hostname | string   | ---     | Host's IP address   | yes             |
| user     | string   | ---     | Host's username     | yes             |
| password | string   | NULL    | Username's password | not             |
| port     | int      | 22      | Host's port         | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Connect to the host using ssh.

```lang-javascript
> var ssh = new Ssh( "192.168.20.71", "sdbadmin", "sdbadmin", 22 )
```