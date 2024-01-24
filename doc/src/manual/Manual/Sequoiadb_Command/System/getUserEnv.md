##名称##

getUserEnv - 获取当前用户的环境变量

##语法##

**System.getUserEnv()**

##类别##

System

##描述##

获取当前用户的环境变量

##参数##

无

##返回值##

返回当前用户的环境变量

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

获取当前用户的环境变量

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