##名称##

updateRole - 更新角色

##语法##

**db.updateRole(\<rolename\>, \<role\>)**

##类别##

Sdb

##描述##

该函数用于更新[自定义角色][user_defined_roles]，会覆盖原定义的权限和继承角色

##参数##

* rolename （ *string，必填* ） 通过rolename指定待更新的角色名：

* role （ *object，必填* ） 通过role指定更新的角色的权限和继承角色：

   * Privileges （ *array* ） 授予角色的权限数组。一个权限由一个Resource和Actions组成。

   * Roles （ *array* ） 该角色继承权限的角色数组。可以包含其他的自定义角色或者[内建角色][builtin_roles]。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6   | SDB_INVALIDARG          | 参数错误 | 检查权限定义是否符合模型 |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | 指定角色不存在 | |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8 及以上版本

##示例##
在集群中更新名为`foo_developer`的角色，使其继承内建角色`_foo.readWrite`，并额外为其授予在集群上的`snapshot`权限

```lang-javascript
> db.updateRole("foo_developer", {
   Privileges:[
      {Resource:{Cluster:true}, Actions:["snapshot"]}
   ],
   Roles:["_foo.readWrite"]
})
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[builtin_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md