
在 SequoiaDB 中，权限是指用户对数据库资源的操作权限，由资源和允许的操作组成。资源是指数据库中的对象，分为集合空间、集合和集群。操作是指对资源的操作。

#权限#

权限是指用户对数据库资源的操作权限，由资源和允许的操作组成。

资源是指数据库中的对象，分为集合空间、集合和集群。如果资源是集群，则附属操作会影响系统的状态，而不是特定的集合空间或集合。

权限的定义格式如下：
```lang-javascript
{
      "Resource" : <resource>,
      "Actions" : [<action>, <action>, ...]
}
```

#资源#

资源指定了权限允许操作的资源。

##集合空间和集合资源##

集合空间和集合资源的定义格式如下：
```lang-javascript
{ "cs" : <collection space name>, "cl" : <collection name> }
```

###指定集合###

如果资源对象同时指定了`cs`与`cl`，该资源是指定集合空间中的指定集合。例如，下列对象定义了一个代表集合空间`foo`中集合`bar`的资源：
```lang-javascript
{ "cs" : "foo", "cl" : "bar" }
```

###指定集合空间###
如果只有`cl`留空，该资源是指定集合空间中的所有集合。例如，下列对象定义了一个代表集合空间`foo`中所有集合的资源：
```lang-javascript
{ "cs" : "foo", cl: "" }
```

###指定跨集合空间的同名集合###
如果只有`cs`留空，该资源是集群中所有集合空间的同名集合。例如，下列对象定义了一个代表集群中所有集合空间中`bar`同名的集合的资源：
```lang-javascript
{ "cs" : "", "cl" : "bar" }
```

###指定非系统集合###

如果`cs`和`cl`都留空，该资源是集群中所有非系统集合的资源。例如，下列对象定义了一个代表集群中所有非系统集合的资源：
```lang-javascript
{ "cs" : "", "cl" : "" }
```

##集群资源##

集群资源的定义格式如下：
```lang-javascript
{ "Cluster" : true }
```

使用集群资源执行影响系统状态的操作，而不是对特定集合空间或集合的执行操作，例如，此类操作的示例包括更新节点配置、管理用户和角色、节点部署和启停等。例如，下列对象授予集群上的更新节点配置操作权限。
```lang-javascript
{
   Resource: { Cluster:true },
   Actions: ["updateConfig"]
}

```

##任何资源##

任何资源提供对集群中每个资源的访问，仅供内部使用，除特殊情况外，请勿使用此资源。任何资源的定义格式如下：
```lang-javascript
{ "AnyResource" : true }
```

## 操作 ##

操作定义了用户可以对资源执行的操作。操作仅定义在匹配的资源上才有效，所有操作类型及命令所需权限映射参考[操作类型][action_types]。SequoiaDB 提供了预定义权限的[内建角色][builtin_roles]，还可定义[用户自定义角色][user_defined_roles]。

[^_^]:
    本文使用的所有引用和链接
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[action_types]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/action_types.md