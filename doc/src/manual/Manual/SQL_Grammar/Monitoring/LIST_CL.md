
集合列表可以列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标识##

$LIST_CL

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Name   | string | 集合完整名 |
| UniqueID | int64 | 集合的 UniqueID，在集群上全局唯一 |
| Version | int32 | 集合的版本号，由 1 起始，每次对该集合的元数据变更会造成版本号 +1|
| Attribute | int32 | 集合的属性掩码，取值可参考 [SYSCOLLECTION 集合][syscollection] |
| AttributeDesc | string | 集合的属性描述，取值可参考 [SYSCOLLECTION 集合][syscollection] |
| CompressionType | int32 | 压缩算法类型掩码，取值可参考 [SYSCOLLECTION 集合][syscollection] |
| CompressionTypeDesc | string | 压缩算法类型描述，取值可参考 [SYSCOLLECTION 集合][syscollection] |
| CataInfo | array | 集合所在的逻辑节点信息 |
| CreateTime | string | 创建集合的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合元数据的时间（仅在 v3.6.1 及以上版本生效） |

##示例##

查看集合列表

```lang-javascript
> db.exec("select * from $LIST_CL")
```

输出结果如下：

```lang-json
{
  "_id": {
    "$oid": "5cf4b6a307c2e1754b77c5ef"
  },
  "Name": "sample.employee",
  "UniqueID": 2469606195201,
  "Version": 1,
  "Attribute": 1,
  "AttributeDesc": "Compressed",
  "CompressionType": 1,
  "CompressionTypeDesc": "lzw",
  "CataInfo": [
    {
      "GroupID": 1001,
      "GroupName": "db2"
    }
  ],
  "CreateTime": "2022-10-06-18.04.31.090000",
  "UpdateTime": "2022-10-06-18.04.31.164000"
}
...
```

[^_^]:
    本文使用的所有引用及链接
[syscollection]:manual/Manual/Catalog_Table/SYSCOLLECTIONS.md

