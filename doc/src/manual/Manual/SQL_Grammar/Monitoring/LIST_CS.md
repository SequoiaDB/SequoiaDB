
集合空间列表可以列出当前数据库节点中所有的集合空间，每个集合空间为一条记录。

##标识##

$LIST_CS

##字段信息##

| 字段名 | 类型   | 描述       |
| ----- | ----- | ----- |
| Collection | array | 包含集合的信息 |
| LobPageSize    | int32   | 集合空间大对象页大小        |
| Name   | string | 集合空间名 |
| PageSize | int32 | 集合空间数据页大小 |
| Type | int32 | 集合空间类型，0 表示普通集合空间，1 表示固定（Capped）集合空间 |
| UniqueID | int32   | 集合空间的 UniqueID，在集群上全局唯一 |
| CreateTime | string | 创建集合空间的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合空间元数据的时间（仅在 v3.6.1 及以上版本生效） |

##示例##

查看集合空间列表

```lang-javascript
> db.exec( "select * from $LIST_CS" )
```

输出结果如下：

```lang-json
{
  "CLUniqueHWM": 2469606195201,
  "Collection": [
    {
      "Name": "employee",
      "UniqueID": 2469606195201
    }
  ],
  "LobPageSize": 262144,
  "Name": "sample",
  "PageSize": 65536,
  "Type": 0,
  "UniqueID": 575,
  "_id": {
    "$oid": "5cf4b69607c2e1754b77c5ee"
  },
  "CreateTime": "2022-10-06-18.04.31.008000",
  "UpdateTime": "2022-10-06-18.05.49.384000"
}
...
```
