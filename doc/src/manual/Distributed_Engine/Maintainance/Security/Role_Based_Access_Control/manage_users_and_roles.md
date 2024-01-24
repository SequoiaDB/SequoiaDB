
SequoiaDB 提供了一系列的命令来管理用户和角色。这些命令包括：
* `createRole()`
* `dropRole()`
* `getRole()`
* `listRoles()`
* `updateRole()`
* `grantPrivilegesToRole()`
* `revokePrivilegesFromRole()`
* `grantRolesToRole()`
* `revokeRolesFromRole()`
* `createUsr()`
* `dropUsr()`
* `getUser()`
* `grantRolesToUser()`
* `revokeRolesFromUser()`
* `invalidateUserCache()`

本教程提供了指导来演示如何在 SequoiaDB 中开启访问控制并创建第一个`_root`用户，以及如何创建其他用户和角色。

##开启访问控制##
通过`updateConf()`或者更改节点配置文件的方式来开启访问控制参数`--privilegecheck`，并重启节点。

##创建第一个超级用户###
```lang-javascript
> db = Sdb()
> db.createUsr("su", "123", {Roles: ["_root"]})
```

##创建自定义角色##

使用超级用户连接，使用`createRole()`命令创建自定义角色`foo_bar_read`，赋予其在集合`foo.bar`上的查询权限。

```lang-javascript
> db = Sdb(<hostname>, <port>, "su", "123")
> db.createRole({
      Role: "foo_bar_read",
      Privileges: [
         {
               Resource: {cs: "foo", cl: "bar"},
               Actions: ["find"]
         }
      ]
   })
```

创建角色`foo_bar_write`，继承角色`foo_bar_read`。

```lang-javascript
> db.createRole({
      Role: "foo_bar_write",
      Privileges: [
         {
               Resource: {cs: "foo", cl: "bar"},
               Actions: ["insert", "update", "remove"]
         }
      ],
      Roles: ["foo_bar_read"]
   })
```

##创建用户##

使用`createUsr()`命令创建用户`myuser1`，赋予其角色`foo_bar_write`。

```lang-javascript
> db.createUsr("myuser1", "123", {Roles: ["foo_bar_write"]})
```

##授予与撤销用户的角色##

使用`revokeRolesFromUser()`命令撤销用户`myuser1`的角色`foo_bar_read`。

```lang-javascript
> db.revokeRolesFromUser("myuser1", ["foo_bar_read"])
```

使用`grantRolesToUser()`命令授予用户`myuser1`角色`foo_bar_read`。

```lang-javascript
> db.grantRolesToUser("myuser1", ["foo_bar_read"])
```

##授予与撤销角色继承的角色##

使用`revokeRolesFromRole()`命令撤销角色`foo_bar_write`继承的角色`foo_bar_read`。

```lang-javascript
> db.revokeRolesFromRole("foo_bar_write", ["foo_bar_read"])
```

使用`grantRolesToRole()`命令授予角色`foo_bar_write`继承的角色`foo_bar_read`。

```lang-javascript
> db.grantRolesToRole("foo_bar_write", ["foo_bar_read"])
```

##授予与撤销角色的权限##

使用`revokePrivilegesFromRole()`命令撤销角色`foo_bar_read`在集合`foo.bar`上的查询权限。

```lang-javascript
> db.revokePrivilegesFromRole("foo_bar_read", [
      {
            Resource: {cs: "foo", cl: "bar"},
            Actions: ["find"]
      }
   ])
```

使用`grantPrivilegesToRole()`命令授予角色`foo_bar_read`在集合`foo.bar`上的查询权限。

```lang-javascript
> db.grantPrivilegesToRole("foo_bar_read", [
      {
            Resource: {cs: "foo", cl: "bar"},
            Actions: ["find"]
      }
   ])
```

