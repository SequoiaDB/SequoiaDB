
访问控制参数为 `--privilegecheck`，该参数为重启生效，默认不开启。
更改配置项参数的方法请参考[参数配置][config]。

访问控制是指对用户访问数据库的权限进行控制。SequoiaDB 通过角色的方式来实现访问控制，用户通过角色来获取权限。
即使不开启访问控制，也可以对用户和角色进行管理，访问控制配置项仅决定是否在执行命令时检查用户的权限。

###访问控制示例###
假设有超级用户`su`与用户`myuser`，此时协调节点`--privilegecheck`未开启，用户`myuser`未被授予任何角色。
已有集合`foo.bar`，使用`myuser`连接，执行`find`命令，可以查询到集合`foo.bar`中的数据。

```lang-javascript
> myuser = Sdb( <hostname>, <port>, "myuser", <pwd>)
> myuser.foo.bar.find()
{
  "_id": {
    "$oid": "64bf968f1d51c64185b134dd"
  },
  "a": 1
}
{
  "_id": {
    "$oid": "64c07d981d51c64185b134de"
  },
  "a": 2
}
```

开启配置项 `--privilegecheck`，重启节点，此时用户`myuser`未被授予任何角色，执行`find`命令将抛出错误`SDB_NO_PRIVILEGES`。

```lang-javascript
> myuser.foo.bar.find()
sdb.js:692 uncaught exception: -393
No privileges for the operation:
No privilege for actions [find] on collection foo.bar
```

使用超级用户`su`连接，为用户`myuser`授予内建角色`_foo.read`，此时用户`myuser`可以执行`find`命令。

```lang-javascript
> su = Sdb( <hostname>, <port>, "su", <pwd>)
> su.grantRolesToUser("myuser", ["_foo.read"])
> myuser.foo.bar.find()
{
  "_id": {
    "$oid": "64bf968f1d51c64185b134dd"
  },
  "a": 1
}
{
  "_id": {
    "$oid": "64c07d981d51c64185b134de"
  },
  "a": 2
}
```

为用户`myuser`撤销内建角色`_foo.read`，用户`myuser`执行`find`命令将抛出错误`SDB_NO_PRIVILEGES`。

```lang-javascript
> su.revokeRolesFromUser("myuser", ["_foo.read"]);
> myuser.foo.bar.find()
sdb.js:692 uncaught exception: -393
No privileges for the operation:
No privilege for actions [find] on collection foo.bar
```


[^_^]:
    本文使用的所有引用和链接
[config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md