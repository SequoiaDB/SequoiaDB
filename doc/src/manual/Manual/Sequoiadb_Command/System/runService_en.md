
##NAME##

runService - Run the service command

##SYNOPSIS##

***System.runService( \<servicename\>, \<command\>, \[option\] )***

##CATEGORY##

System

##DESCRIPTION##

Run the service command

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| servicename   | string   | --- | service program name   | yes    |
| command     | string   | ---   | the command corresponding to service name   | yes    |
| option | string  | NULL    | command option | not     |

##RETURN VALUE##

On success, return the information of service command execution.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

View ssh service information

```lang-javascript
> System.runService( "ssh", "status", "" )
● ssh.service - OpenBSD Secure Shell server
   Loaded: loaded (/lib/systemd/system/ssh.service; enabled; vendor preset: enabled)
      Active: active (running) since 三 2019-05-29 10:29:47 CST; 6 days ago
Main PID: 1637 (sshd)
   Tasks: 1
   Memory: 5.6M
      CPU: 1.268s
   CGroup: /system.slice/ssh.service
            └─1637 /usr/sbin/sshd -D

6月 04 14:57:06 hostname sshd[24292]: pam_unix(sshd:session): session opened for user sdbadmin by (uid=0)
6月 04 15:12:22 hostname sshd[17900]: pam_unix(sshd:auth): authentication failure; logname= uid=0 euid=0 tty=ssh ruser= rhost=192.168.10.124  user=username
```