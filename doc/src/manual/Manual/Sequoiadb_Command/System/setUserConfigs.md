##名称##

setUserConfigs - 修改操作系统用户的配置

##语法##

**System.setUserConfigs(\<options\>)**

##类别##

System

##描述##

该函数用于修改操作系统用户的用户组、附加组、用户目录等配置。

##参数##

options（ *object，必填* ）

通过参数 options 可以修改用户的属性：

- name（ *string* ）：指定需要修改的用户，该参数必填

    格式：`name: "username"`

- gid（ *string* ）：指定用户的初始组(主组)

    该参数可以是用户组的组名或组 ID，且指定的用户组必须已存在。如果不指定，则默认创建与参数 name 同名的用户组。

    格式：`gid: "groupName"` 或 `gid: "2003"`

- groups（ *string* ）：指定附加组

    该参数可以是用户组的组名或组 ID，且指定的用户组必须已存在。所指定的多个用户组以逗号分隔。

    格式：`groups: "groupName1,groupName2,groupName3"` 或 `groups: "2004,2005,2006"`


- isAppend（ *boolean* ）：指定是否追加附加组，默认为 false

    该参数需要配合参数 groups 使用。当指定了 groups 且设置 isAppend 为 true，将会追加该用户的附加组；当指定了 groups 且设置 isAppend 为 false，将会替换原有的附加组。

    格式：`isAppend: true`

- isMove（ *boolean* ）：指定是否移动到新指定的目录，默认为 false

    该参数为 ture 时必须指定参数 dir 的值。

    格式：`isMove: true`

- dir（ *string* ）：指定新的用户目录，仅参数 isMove 为 true 时生效

    该参数不能指定已存在的目录。指定的新目录将保留原用户目录的数据，而原用户目录将会被移除。

    格式：`dir: "userHomeDir"`


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2 及以上版本

##示例##

修改指定用户组中 `newUser` 用户的 home 目录

```lang-javascript
> System.setUserConfigs({name: "newUser", gid: "groupName", dir: "/home/userName", isMove: true})
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md