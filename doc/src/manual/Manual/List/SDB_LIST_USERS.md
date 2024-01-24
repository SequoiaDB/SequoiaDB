

用户列表可以列出当前集群中的所有用户信息。

##标识##

SDB_LIST_USERS

##字段信息##

| 字段名             | 类型   | 描述               |
| ----------------  | ------ | ------------------ |
| User              | string  | 用户名             |
| Options.AuditMask | string  | 用户审计日志配置掩码 |

##示例##

查看用户列表

```lang-javascript
> db.list( SDB_LIST_USERS )
```

输出结果如下：

```lang-json
{
  "User": "admin",
  "Options": {}
}
{
  "User": "user2",
  "Options": {
    "AuditMask": "DDL|DML|!DQL"
  }
}
```
