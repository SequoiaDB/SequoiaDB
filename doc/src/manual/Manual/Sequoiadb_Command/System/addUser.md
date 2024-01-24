##名称##

addUser - 新增操作系统用户

##语法##

**System.addUser(\<users\>)**

##类别##

System

##描述##

该函数用于新增操作系统用户。

##参数##

users（ *object，必填* ）

通过参数 users 可以设置用户的属性：

- name（ *string* ）：用户名，该参数必填

    格式：`name: "username"`

- gid（ *string* ）：指定用户的初始组(主组)

    该参数可以是用户组的组名或组 ID，且指定的用户组必须已存在。如果不指定，则默认创建与参数 name 同名的用户组。

    格式：`gid: "groupName"` 或 `gid: "2003"`

- groups（ *string* ）：指定附加组

    该参数可以是用户组的组名或组 ID，且指定的用户组必须已存在。所指定的多个用户组以逗号分隔。

    格式：`groups: "groupName1,groupName2,groupName3"` 或 `groups: "2004,2005,2006"`

- createDir（ *boolean* ）：是否创建用户目录，默认为 false

    格式：`createDir: true`

- dir（ *string* ）：指定用户目录，仅参数 createDir 为 true 时生效

    该参数不能指定已存在的目录。如果不指定，将会在 `/home` 目录下创建与参数 name 同名的目录作为用户目录。

    格式：`dir: "userHomeDir"`


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2 及以上版本

##示例##

新增名为“newUser”的系统用户，并指定用户组为 root，同时创建用户目录 `/home/newUser`

```lang-javascript
> System.addUser({name: "newUser", gid: "root", createDir: true, dir: "/home/newUser"})
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md