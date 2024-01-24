SequoiaDB 巨杉数据库审计日志记录了用户对数据库执行的所有操作。通过审计日志，用户可以对数据库进行故障分析、行为分析和安全审计等操作，能有效帮助用户获取数据库的执行情况。

审计层级
----

SequoiaDB 支持配置节点和用户两种层级的审计功能。

* 节点级审计功能：表示对节点内所有的用户和集合都是采用统一的审计配置

* 用户级审计功能：表示该用户的所有操作优先使用该用户的审计配置

用户通过配置[审计操作类型][auditlog]可以决定日志输出的审计信息。用户级的审计配置优先级高于节点级审计配置，当用户同时配置多级审计操作类型时，未配置的项将继承优先级较低的审计配置项。例如：节点级审计配置为 DDL，用户级审计配置为 DML，则该用户的生效审计配置为"DDL | DML"；如果用户级审计配置为"!DDL | DML"，则该用户的生效审计配置为"DML"。

> **Note：**
> 
>  SequoiaDB 集群模式支持所有级别的审计日志，独立模式仅支持节点级审计日志。

##节点级审计日志##

###开启审计功能###

SequoiaDB 默认开启节点级审计功能，默认审计操作类型为"SYSTEM|DDL|DCL"。

###查看审计配置###

用户可以使用 snapshot() 命令查看审计配置

```lang-javascript
db.snapshot(SDB_SNAP_CONFIGS, {}, {NodeName:"", auditpath:"", auditnum:"", auditmask:""})
```

输出结果如下：

```lang-json
{ "NodeName": "sdbserver:30000", "auditpath": "/opt/sequoiadb/30000/diaglog/", "auditnum": 20, "auditmask": "SYSTEM|DDL|DCL" }
{ "NodeName": "sdbserver:30010", "auditpath": "/opt/sequoiadb/30010/diaglog/", "auditnum": 20, "auditmask": "SYSTEM|DDL|DCL" }
{ "NodeName": "sdbserver:30020", "auditpath": "/opt/sequoiadb/30020/diaglog/", "auditnum": 20, "auditmask": "SYSTEM|DDL|DCL" }
{ "NodeName": "sdbserver:50000", "auditpath": "/opt/sequoiadb/50000/diaglog/", "auditnum": 20, "auditmask": "SYSTEM|DDL|DCL" }
```

- auditpath 表示审计日志文件路径，默认为`数据文件路径/diaglog/sdbaudit.log`
- auditnum 表示审计日志文件个数，默认为 20
- auditmask 表示审计日志审计操作类型，默认为"SYSTEM|DDL|DCL"

###修改审计操作类型###

用户可以执行 [updateConf()][updateConf] 命令修改 auditmask 取值以修改审计操作类型，该操作动态生效。

修改 auditmask 取值为"SYSTEM|DDL|DCL|DQL"

```lang-javascript
> db.updateConf({auditmask:"SYSTEM|DDL|DCL|DQL"}) 
```

> **Note：**
>
> SequoiaDB 巨杉数据库 v3.0 及以上版本配置节点"auditmask"后在线动态生效，v3.0 以下版本需要重启节点生效，详情可参考[配置项参数][configuration_parameters]章节 auditmask 参数说明。

###关闭审计功能###

用户可通过 [updateConf()][updateConf] 命令修改 auditMask 取值为"NONE"以关闭节点级审计功能。

```lang-javascript
> db.updateConf({auditmask:"NONE"}) 
```

##用户级审计日志##

###开启审计功能###

SequoiaDB 默认关闭用户级审计功能，用户可通过 [createUsr()][createUsr] 命令开启用户级审计功能。

用户可执行 createUsr() 命令配置 AuditMask，并重新登录用户使配置生效。
    
创建 admin 用户，配置用户名为"admin"，密码为"admin"，审计操作类型 AuditMask 为"DDL|DML|!DQL"

```lang-javascript
> db.createUsr("admin", "admin", {AuditMask: "DDL|DML|!DQL"})
```

###查看审计配置###

用户可以通过[用户列表][SDB_LIST_USERS]查看指定用户的 AuditMask 取值。

查看 sample 用户的审计操作类型 AuditMask 配置

```lang-javascript
> db.list(SDB_LIST_USERS)
```

输出结果如下：

```lang-json
{
    "User": "sample"
    "Options": {
        "AuditMask": "DDL|DML|!DQL"
    }
}
```

###关闭审计功能###

用户可以通过 [dropUsr()][dropUsr] 删除指定用户来关闭用户级审计功能。

删除用户名为"admin"，密码为"admin"的数据库权限

```lang-javascript
> db.dropUsr("admin","admin")
 ```

审计日志解析
----

SequoiaDB 只能查看节点路径下的审计日志文件，`sdbaudit.log` 审计日志内容所包含字段说明如下：

| 字段       | 说明                                                |
| ---------- | --------------------------------------------------- |
| Type       | 操作类型，如"DML"                                   |
| PID        | 进程号                                              |
| TID        | 线程号                                              |
| UserName   | 操作用户，如"admin"                                 |
| From       | 操作地址，如"192.168.11.11:88888"                   |
| Action     | 操作，如"INSERT"                                    |
| Result     | 操作结果，如"SUCCEED(0)" ，其中括号内的数字为操作结果返回码|
| ObjectType | 对象类型，如"COLLECTION"                            |
| ObjectName | 对象名，如"sample.employee"                         |
| Message    | 详细信息                                            |

> **Note：**
>
> Result 操作结果中的返回码，0 表示操作成功，非 0 表示操作失败（如 -33 创建集合失败）。非 0 返回码对应解读信息可参考[错误码][error_code]。


创建集合 sample.employee，查看当前 coord 节点数据目录下的 `sdbaudit.log` 审计日志文件，内容如下：

```
2018-08-24-17.45.49.444138               Type:DDL
PID:7479                                 TID:5011
UserName:                                From:192.168.3.24:32974
Action:COMMAND(create collection)        Result:SUCCEED(0)
ObjectType:COLLECTION                    ObjectName:sample.employee
Message:
Option: { "Name": "sample.employee" }
```

审计操作类型
----

审计操作类型分为操作审计和数据审计，可配置的审计操作类型如下：

###操作审计###

| 操作类型 | 说明                                                         |
| -------- | ------------------------------------------------------------ |
| ACCESS   | 登入登出                                                     |
| CLUSTER  | 集群操作，支持的操作包括创建组和删除组                         |
| SYSTEM   | 系统操作，支持的操作包括重启节点                               |
| DCL      | 数据控制操作，支持的操作包括创建用户、修改用户和删除用户           |
| DDL      | 数据定义操作，支持的操作包括创建集合空间、修改集合空间、删除集合空间、创建集合、修改集合及删除集合 |
| DML      | 数据库操作，支持的操作包括插入数据、更新数据和删除数据           |
| DQL      | 数据查询操作                                                     |
| OTHER    | 其他，以上类型之外的操作                                     |

> **Note:**
>
> 当用户配置 DML 时，仅记录具体操作，而不记录具体的操作数据。

###数据审计###

| 操作类型 | 说明                                                         |
| -------- | ------------------------------------------------------------ |
| INSERT   | 插入数据，记录每条插入语句所操作的具体数据                   |
| UPDATE   | 更新数据，记录每条更新语句所操作的具体数据                   |
| DELETE   | 删除数据，记录每条删除语句所操作的具体数据                   |

> **Note：**
>
> - 支持使用"|"连接多个操作类型，如"DDL|DML|DQL"；
> - 支持使用"!"禁止某个操作类型，如"!DCL|DML"；此设置仅允许用户级配置使用；
> - 用户修改配置生效后可执行 [invalidateCache()][invalidateCache] 命令清除节点缓存。



[^_^]:
     本文使用的所有链接和引用
[invalidateCache]:manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md
[updateConf]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
[createUsr]:manual/Manual/Sequoiadb_Command/Sdb/createUsr.md
[SDB_LIST_USERS]:manual/Manual/List/SDB_LIST_USERS.md
[dropUsr]:manual/Manual/Sequoiadb_Command/Sdb/dropUsr.md
[configuration_parameters]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md#审计操作类型
