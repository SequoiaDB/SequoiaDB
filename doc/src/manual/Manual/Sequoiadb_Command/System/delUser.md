##名称##

delUser - 删除操作系统用户

##语法##

**System.delUser(\<users\>)**

##类别##

System

##描述##

该函数用于删除操作系统用户。

##参数##

users（ *object，必填* ）

通过参数 users 可以设置需要删除的用户：

- name（ *string* ）：用户名，该参数必填

    格式：`name: "username"`

- isRemoveDir（ *boolean* ）：是否删除用户目录，默认为 false

    格式：`isRemoveDir: true`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2 及以上版本

##示例##

删除指定的系统用户

```lang-javascript
> System.delUser({name: "newUser"})
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md