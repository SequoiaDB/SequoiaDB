

SYSCAT.SYSDOMAINS 集合中包含了该集群中所有的用户域信息，每个用户域保存为一个文档。

每个文档包含以下字段：

|  字段名     |   类型    |    描述                                  |
|-------------|-----------|------------------------------------------|
|  Name       |   string  | 域的名称                               |
|  Groups     |   array   | 该域中包含的所有的数据复制组，每个复制组表示为一个嵌套的 JSON 对象，包含 GroupName 字段与  GroupID 字段表示复制组的名称和 ID 标识 |
|  AutoSplit  |   boolean | 当此属性为 true 时，在域上创建的散列分区集合会被自动切分至该域包含的所有复制组上 |

##示例##

包含一个复制组的域信息如下：

```lang-json
{
  "Name" : "Domain",
  "Groups" : [ { "GroupName" : "group1", "GroupID" : 1000 } ],
  "AutoSplit" : true
}
```

