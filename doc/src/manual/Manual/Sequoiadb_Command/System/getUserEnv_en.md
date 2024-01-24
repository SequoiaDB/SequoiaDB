
##NAME##

getUserEnv - Acquire the environment variable for the current user

##SYNOPSIS##

***System.getUserEnv()***

##CATEGORY##

System

##DESCRIPTION##

Acquire the environment variable for the current user

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the environment variable for the current user.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire the environment variable for the current user

```lang-javascript
> System.getUserEnv()
{
  "MAIL": "/var/mail/name",
  "SSH_CLIENT": "192.168.10.124 49706 22",
  "USER": "name",
  "LANGUAGE": "zh_CN:zh",
  "SHLVL": "1",
  "HOME": "/home/users/name",
  "OLDPWD": "/opt/trunk/doc/config",
  "SSH_TTY": "/dev/pts/8",
  "LOGNAME": "name",
  "_": "bin/sdb",
  "XDG_SESSION_ID": "1518",
  "TERM": "xterm",
  "PATH": "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin",
  "XDG_RUNTIME_DIR": "/run/user/2160",
  "LANG": "zh_CN.UTF-8",
  "SHELL": "/bin/bash",
  "PWD": "/opt/trunk",
  "XDG_DATA_DIRS": "/usr/local/share:/usr/share:/var/lib/snapd/desktop",
  "SSH_CONNECTION": "192.168.10.124 49706 192.168.20.62 22"
}
```