
SequoiadDB 的用户在旧版本中仅有角色`admin`与`monitor`。

升级到基于角色的访问控制版本后，为保证兼容性，用户的角色有如下几个规则：

1. 已有的旧版本的用户角色`admin`，被视作拥有内建角色为`_root`; 旧版本的用户角色`monitor`，被视作拥有内建角色为`_clusterMonitor`。
2. 新版本中`createUsr()`命令仍然支持通过 `Options.Role` 字段为用户指定旧版本角色，视作拥有内建角色为`_root`或`_clusterMonitor`。
3. 删除用户时，如果集群中除待删除用户外，不存在任何一个用户拥有内建角色`_root`或者旧版本`admin`, 将会删除失败

##用户升级的流程##
假设在旧版本中有`admin`用户与`monitor`用户。

1. 升级节点并重启，节点的`--privilegecheck`配置项默认应为关闭。
2. 使用`admin`用户连接，创建新用户`su`，并为其授予内建角色`_root`。

    ```lang-javascript
    > db = Sdb( <hostname>, <port>, "admin", <pwd>)
    > db.createUsr("su", <pwd>, {Roles: ["_root"]})
    ```
3. 更新节点配置，开启`--privilegecheck`配置项，重启节点。
4. 业务系统使用旧版本中的用户仍然拥有与旧版本中相同的权限，业务系统不会受到影响。
5. 使用`su`用户连接，根据业务系统的实际需求，创建更多基于角色的访问控制用户。
6. 迁移业务系统到新版本，使用新版本中的用户进行操作。
7. 业务系统迁移完成后，删除旧版本中的用户`admin`与`monitor`。