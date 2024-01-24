##名称##

revokeRolesFromUser - 为用户撤销角色

##语法##

**db.revokeRolesFromUser(\<username\>, \<roles\>)**

##类别##

Sdb

##描述##

该函数用于为用户撤销[自定义角色][user_defined_roles]和[内建角色][builtin_roles]。

##参数##

* username （ *string，必填* ） 指定待更新的用户名：

* roles （ *array，必填* ） 为用户撤销的角色数组。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | 指定角色不存在 | |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8 及以上版本

##示例##
在集群中为用户`user1`撤销内建角色和自定义角色。

```lang-javascript
> db.revokeRolesFromUser("user1",["_foo.admin","other_role"])
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[builtin_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md