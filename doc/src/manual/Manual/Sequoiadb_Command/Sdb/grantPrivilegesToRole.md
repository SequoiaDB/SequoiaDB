##名称##

grantPrivilegesToRole - 授予角色新的权限

##语法##

**db.grantPrivilegesToRole(\<rolename\>, \<privileges\>)**

##类别##

Sdb

##描述##

该函数用于授予[自定义角色][user_defined_roles]新的权限。

##参数##

* rolename （ *string，必填* ） 通过rolename指定待更新的角色名：

* Privileges （ *array，必填* ） 授予角色的权限数组。一个权限由一个Resource和Actions组成。

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
在集群中授予名为`foo_developer`的角色新的权限。

```lang-javascript
> db.grantPrivilegesToRole("foo_developer",[
      {Resource:{cs: "foo", cl: ""}, Actions:["createCL", "dropCL"]}
   ])
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[builtin_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md