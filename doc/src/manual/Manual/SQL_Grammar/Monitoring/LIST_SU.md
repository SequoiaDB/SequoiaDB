
存储单元列表可以列出当前数据库节点的全部存储单元信息。

>   **Note:**
>
>   该列表只能在数据节点和编目节点执行。

##标识##

$LIST_SU


##字段信息##

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| NodeName       | string | 所在的节点名                |
| Name           | string | 集合空间名                  |
| UniqueID       | int32   | 集合空间的 UniqueID，在集群上全局唯一 |
| ID             | int32   | 该集合空间 ID               |
| LogicalID      | string  | 集合空间逻辑 ID，为递增顺序 |
| PageSize       | int32   | 集合空间数据页大小          |
| LobPageSize    | int32   | 集合空间大对象页大小        |
| Sequence       | int32   | 序列号，当前版本中为 1      |
| NumCollections | int32   | 集合空间下的集合个数        |
| CollectionHWM  | int32   | 集合高水位，一般来说意味着该集合空间中总共创建过的集合数量（包括被删除的集合） |
| Size           | int64 | 存储单元大小，单位为字节      |
| CreateTime | string | 创建集合空间的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合空间元数据的时间（仅在 v3.6.1 及以上版本生效） |

##示例##

查看存储单元列表

```lang-javascript
> db.exec( "select * from $LIST_SU" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:30000",
  "Name": "SYSAUTH",
  "UniqueID": 0,
  "ID": 5,
  "LogicalID": 5,
  "PageSize": 65536,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 1,
  "CollectionHWM": 1,
  "Size": 306315264,
  "CreateTime": "2022-10-06-18.04.31.008000",
  "UpdateTime": "2022-10-06-18.05.49.384000"
}
{
  "NodeName": "hostname:30000",
  "Name": "SYSCAT",
  "UniqueID": 0,
  "ID": 1,
  "LogicalID": 1,
  "PageSize": 65536,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 6,
  "CollectionHWM": 6,
  "Size": 306315264,
  "CreateTime": "2022-10-06-18.04.31.090000",
  "UpdateTime": "2022-10-06-18.04.31.164000"
}
...
```
